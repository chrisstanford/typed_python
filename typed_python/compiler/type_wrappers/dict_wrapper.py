#   Copyright 2017-2020 typed_python Authors
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from typed_python.compiler.type_wrappers.refcounted_wrapper import RefcountedWrapper
from typed_python.compiler.typed_expression import TypedExpression
from typed_python.compiler.conversion_level import ConversionLevel
import typed_python.compiler.type_wrappers.runtime_functions as runtime_functions
from typed_python.compiler.type_wrappers.bound_method_wrapper import BoundMethodWrapper
from typed_python.compiler.type_wrappers.hash_table_implementation import table_next_slot, table_clear, \
    dict_table_contains, dict_delitem, dict_getitem, dict_get, dict_setitem
from typed_python import Tuple, PointerTo, Int32, UInt8, Dict, ConstDict, TypeFunction, Held, Class, Member, Final

import typed_python.compiler.native_ast as native_ast
import typed_python.compiler


typeWrapper = lambda t: typed_python.compiler.python_object_representation.typedPythonTypeToTypeWrapper(t)


def checkDictSizeAndThrowIfChanged(instance, expectedLen):
    if len(instance) != expectedLen:
        raise RuntimeError("dictionary size changed during iteration")


def dict_update(instance, other):
    for key in other:
        instance[key] = other[key]


def dict_setdefault_nodefault(dict, item):
    if item not in dict:
        dict[item] = type(dict).ValueType()
    return dict[item]


def dict_setdefault(dict, item, defaultValue):
    if item not in dict:
        dict[item] = defaultValue
    return dict[item]


def initialize_dict_from_mappable(ptrToOutDict, mappable, mayThrow):
    """Initialize a dictionary from an arbitrary object supporting the mapping protocol.

    This is called internally by the compiler to support initializing
    a Dict(T1, T2) from another Dict with different key/value types.

    Args:
        ptrToOutDict - a pointer to an uninitialized Dict instance.
        mappable - something we can iterate over with a getitem.
    Returns:
        True if we succeeded, and ptrToOutDict is valid. False if
        we failed, and ptrToOutDict will not point to an initialized
        dictionary.
    """
    ptrToOutDict.initialize()

    try:
        for m in mappable:
            ptrToOutDict.get()[m] = mappable[m]

        return True
    except: # noqa
        ptrToOutDict.destroy()
        if mayThrow:
            raise

    return False


def dict_pop_nodefault(dict, item):
    if item not in dict:
        raise KeyError(item)

    res = dict[item]

    del dict[item]

    return res


def dict_pop(dict, item, defaultValue):
    if item not in dict:
        return defaultValue

    res = dict[item]
    del dict[item]
    return res


def dict_duplicate(d):
    out = type(d)()

    for k in d.items():
        out[k[0]] = k[1]

    return out


class DictWrapperBase(RefcountedWrapper):
    is_pod = False
    is_empty = False
    is_pass_by_ref = True

    CAN_BE_NULL = False

    def __init__(self, t, behavior):
        assert hasattr(t, '__typed_python_category__')
        super().__init__(t if behavior is None else (t, behavior))

        self.keyType = typeWrapper(t.KeyType)
        self.valueType = typeWrapper(t.ValueType)
        self.itemType = typeWrapper(Tuple(t.KeyType, t.ValueType))
        self.dictType = t

        self.kvBytecount = self.keyType.getBytecount() + self.valueType.getBytecount()
        self.keyBytecount = self.keyType.getBytecount()

        self.layoutType = native_ast.Type.Struct(element_types=(
            ('refcount', native_ast.Int64),
            ('items', native_ast.UInt8Ptr),
            ('items_populated', native_ast.UInt8Ptr),
            ('items_reserved', native_ast.Int64),
            ('top_item_slot', native_ast.Int64),
            ('hash_table_slots', native_ast.Int32Ptr),
            ('hash_table_hashes', native_ast.Int32Ptr),
            ('hash_table_size', native_ast.Int64),
            ('hash_table_count', native_ast.Int64),
            ('hash_table_empty_slots', native_ast.Int64),
            ('setdefault', native_ast.Int64)
        ), name="DictWrapper").pointer()

    def on_refcount_zero(self, context, instance):
        assert instance.isReference

        return (
            context.converter.defineNativeFunction(
                "destructor_" + str(self.typeRepresentation),
                ('destructor', self),
                [self],
                typeWrapper(type(None)),
                self.generateNativeDestructorFunction
            ).call(instance)
        )

    def getNativeLayoutType(self):
        return self.layoutType

    def convert_len_native(self, expr):
        if isinstance(expr, TypedExpression):
            expr = expr.nonref_expr
        return expr.ElementPtrIntegers(0, 8).load().cast(native_ast.Int64)


class DictWrapper(DictWrapperBase):
    def __init__(self, dictType):
        super().__init__(dictType, None)

    def has_fastnext_iter(self):
        """If we call '__iter__' on instances of this type, will they support __fastnext__?"""
        return True

    def convert_default_initialize(self, context, instance):
        context.pushEffect(
            instance.expr.store(
                runtime_functions.table_create.call().cast(self.layoutType)
            )
        )

    def convert_attribute(self, context, expr, attr):
        if attr in (
                "__iter__",
                "getItemByIndexUnsafe", "getItemPtrByIndexUnsafe",
                "getKeyByIndexUnsafe", "getKeyPtrByIndexUnsafe",
                "getValueByIndexUnsafe", "getValuePtrByIndexUnsafe",
                "deleteItemByIndexUnsafe",
                "initializeValueByIndexUnsafe", "assignValueByIndexUnsafe",
                "initializeKeyByIndexUnsafe", "_allocateNewSlotUnsafe", "_resizeTableUnsafe",
                "_top_item_slot", "_compressItemTableUnsafe", "get", "items", "keys", "values", "setdefault",
                "pop", "clear", "copy", "update"):
            return expr.changeType(BoundMethodWrapper.Make(self, attr))

        if attr == '_items_populated':
            return context.pushPod(
                PointerTo(UInt8),
                expr.nonref_expr.ElementPtrIntegers(0, 2).load()
            )

        if attr == '_items_reserved':
            return context.pushPod(
                int,
                expr.nonref_expr.ElementPtrIntegers(0, 3).load()
            )

        if attr == '_top_item_slot':
            return context.pushPod(
                int,
                expr.nonref_expr.ElementPtrIntegers(0, 4).load()
            )

        if attr == '_hash_table_slots':
            return context.pushPod(
                PointerTo(Int32),
                expr.nonref_expr.ElementPtrIntegers(0, 5).load()
            )

        if attr == '_hash_table_hashes':
            return context.pushPod(
                PointerTo(Int32),
                expr.nonref_expr.ElementPtrIntegers(0, 6).load()
            )

        if attr == '_hash_table_size':
            return context.pushPod(
                int,
                expr.nonref_expr.ElementPtrIntegers(0, 7).load()
            )

        if attr == '_hash_table_count':
            return context.pushPod(
                int,
                expr.nonref_expr.ElementPtrIntegers(0, 8).load()
            )

        if attr == '_hash_table_empty_slots':
            return context.pushPod(
                int,
                expr.nonref_expr.ElementPtrIntegers(0, 9).load()
            )

        return super().convert_attribute(context, expr, attr)

    @staticmethod
    def supportsMappingProtocol(T):
        """Is T compiled and supporting the mapping protocol?"""
        if issubclass(T, (Dict, ConstDict)):
            return True

        # when we support iterators in Class instances, we can revisit this:
        # if issubclass(T, Class) and '__getitem__' in T.Methods and '__iter__' in T.Methods
        #    return False

        return False

    def _can_convert_from_type(self, otherType, conversionLevel):
        if (
            conversionLevel.isImplicitContainersOrHigher()
            and DictWrapper.supportsMappingProtocol(otherType.typeRepresentation)
        ):
            return "Maybe"

        return super()._can_convert_from_type(otherType, conversionLevel)

    def convert_to_self_with_target(self, context, targetVal, sourceVal, conversionLevel, mayThrowOnFailure=False):
        if conversionLevel.isImplicitContainersOrHigher() and DictWrapper.supportsMappingProtocol(sourceVal.expr_type.typeRepresentation):
            res = context.call_py_function(
                initialize_dict_from_mappable,
                (targetVal.asPointer(), sourceVal, context.constant(mayThrowOnFailure)),
                {}
            )

            return res

        return super().convert_to_self_with_target(context, targetVal, sourceVal, conversionLevel, mayThrowOnFailure)

    def convert_set_attribute(self, context, instance, attr, expr):
        if attr == '_hash_table_count':
            val = expr.toInt64()
            if val is None:
                return None
            context.pushEffect(
                instance.nonref_expr.ElementPtrIntegers(0, 8).store(val.nonref_expr)
            )

            return context.pushVoid()

        if attr == '_top_item_slot':
            val = expr.toInt64()
            if val is None:
                return None

            context.pushEffect(
                instance.nonref_expr.ElementPtrIntegers(0, 4).store(val.nonref_expr)
            )

            return context.pushVoid()

        if attr == '_hash_table_empty_slots':
            val = expr.toInt64()
            if val is None:
                return None
            context.pushEffect(
                instance.nonref_expr.ElementPtrIntegers(0, 9).store(val.nonref_expr)
            )

            return context.pushVoid()

        return super().convert_set_attribute(context, instance, attr, expr)

    def convert_method_call(self, context, instance, methodname, args, kwargs):
        if kwargs:
            return super().convert_method_call(context, instance, methodname, args, kwargs)

        if (methodname == "__iter__" or methodname == "keys") and not args and not kwargs:
            itType = DictKeysIterator(
                self.keyType.typeRepresentation,
                self.valueType.typeRepresentation
            )

            return typeWrapper(itType).convert_type_call(
                context,
                None,
                [instance],
                {}
            )

        if methodname == "values" and not args and not kwargs:
            itType = DictValuesIterator(
                self.keyType.typeRepresentation,
                self.valueType.typeRepresentation
            )

            return typeWrapper(itType).convert_type_call(
                context,
                None,
                [instance],
                {}
            )

        if methodname == "items" and not args and not kwargs:
            itType = DictItemsIterator(
                self.keyType.typeRepresentation,
                self.valueType.typeRepresentation
            )

            return typeWrapper(itType).convert_type_call(
                context,
                None,
                [instance],
                {}
            )

        if len(args) == 0:
            if methodname == "copy":
                return context.push(
                    typeWrapper(self.dictType),
                    lambda ref: ref.expr.store(
                        runtime_functions.table_copy.call(
                            instance.nonref_expr.cast(native_ast.VoidPtr),
                            context.getTypePointer(self.dictType)
                        ).cast(self.layoutType)
                    )
                )
            if methodname == "_compressItemTableUnsafe":
                context.pushEffect(
                    runtime_functions.table_compress.call(
                        instance.nonref_expr.cast(native_ast.VoidPtr),
                        context.constant(self.kvBytecount)
                    )
                )
                return context.pushVoid()

            if methodname == "_resizeTableUnsafe":
                context.pushEffect(
                    runtime_functions.table_resize.call(
                        instance.nonref_expr.cast(native_ast.VoidPtr)
                    )
                )
                return context.pushVoid()

            if methodname == "_allocateNewSlotUnsafe":
                return context.pushPod(
                    Int32,
                    runtime_functions.table_allocate_new_slot.call(
                        instance.nonref_expr.cast(native_ast.VoidPtr),
                        context.constant(self.kvBytecount)
                    )
                )

        if methodname == "setdefault":
            if len(args) == 1:
                return context.call_py_function(dict_setdefault_nodefault, (instance, args[0]), {})
            else:
                return context.call_py_function(dict_setdefault, (instance, args[0], args[1]), {})

        if methodname == "pop":
            if len(args) == 1:
                return context.call_py_function(dict_pop_nodefault, (instance, args[0]), {})
            else:
                return context.call_py_function(dict_pop, (instance, args[0], args[1]), {})

        if methodname == "clear":
            if len(args) == 0:
                return context.call_py_function(table_clear, (instance,), {})

        if methodname == "update":
            if len(args) == 1:
                return context.call_py_function(dict_update, (instance, args[0]), {})

        if len(args) == 1:
            if methodname == "get":
                return self.convert_get(context, instance, args[0], context.constant(None))

            if methodname in (
                "getItemByIndexUnsafe",
                "getKeyByIndexUnsafe",
                "getValueByIndexUnsafe",
                "getItemPtrByIndexUnsafe",
                "getKeyPtrByIndexUnsafe",
                "getValuePtrByIndexUnsafe",
                "deleteItemByIndexUnsafe"
            ):
                index = args[0].toInt64()
                if index is None:
                    return None

                item = context.pushReference(
                    self.itemType,
                    instance.nonref_expr.ElementPtrIntegers(0, 1).load().cast(
                        self.itemType.getNativeLayoutType().pointer()
                    ).elemPtr(index.toInt64().nonref_expr)
                )

                if methodname == "deleteItemByIndexUnsafe":
                    item.convert_destroy()
                    return context.pushVoid()
                elif methodname == "getItemByIndexUnsafe":
                    return item
                elif methodname == "getItemPtrByIndexUnsafe":
                    return item.asPointer()
                elif methodname == "getKeyByIndexUnsafe":
                    # take the first item in the tuple
                    return item.expr_type.refAs(context, item, 0)
                elif methodname == "getKeyPtrByIndexUnsafe":
                    # take the first item in the tuple
                    return item.expr_type.refAs(context, item, 0).asPointer()
                elif methodname == "getValueByIndexUnsafe":
                    # take the first item in the tuple
                    return item.expr_type.refAs(context, item, 1)
                elif methodname == "getValuePtrByIndexUnsafe":
                    # take the first item in the tuple
                    return item.expr_type.refAs(context, item, 1).asPointer()
                else:
                    raise Exception("unreachable")

        if len(args) == 2:
            if methodname == "get":
                return self.convert_get(context, instance, args[0], args[1])
            if methodname in ("initializeValueByIndexUnsafe", 'assignValueByIndexUnsafe'):
                index = args[0].toInt64()
                if index is None:
                    return None

                value = args[1].convert_to_type(self.valueType, ConversionLevel.Implicit)
                if value is None:
                    return None

                item = context.pushReference(
                    self.itemType,
                    instance.nonref_expr.ElementPtrIntegers(0, 1).load().cast(
                        self.itemType.getNativeLayoutType().pointer()
                    ).elemPtr(index.toInt64().nonref_expr)
                )

                if methodname == 'assignValueByIndexUnsafe':
                    item.expr_type.refAs(context, item, 1).convert_assign(value)
                else:
                    item.expr_type.refAs(context, item, 1).convert_copy_initialize(value)

                return context.pushVoid()

            if methodname == "initializeKeyByIndexUnsafe":
                index = args[0].toInt64()
                if index is None:
                    return None

                key = args[1].convert_to_type(self.keyType, ConversionLevel.UpcastContainers)
                if key is None:
                    return None

                item = context.pushReference(
                    self.itemType,
                    instance.nonref_expr.ElementPtrIntegers(0, 1).load().cast(
                        self.itemType.getNativeLayoutType().pointer()
                    ).elemPtr(index.toInt64().nonref_expr)
                )

                item.expr_type.refAs(context, item, 0).convert_copy_initialize(key)

                return context.pushVoid()

        return super().convert_method_call(context, instance, methodname, args, kwargs)

    def convert_delitem(self, context, expr, item):
        if item is None or expr is None:
            return None

        item = item.convert_to_type(self.keyType, ConversionLevel.UpcastContainers)
        if item is None:
            return None

        return context.call_py_function(dict_delitem, (expr, item), {})

    def convert_getitem(self, context, expr, item):
        if item is None or expr is None:
            return None

        item = item.convert_to_type(self.keyType, ConversionLevel.UpcastContainers)
        if item is None:
            return None

        return context.call_py_function(dict_getitem, (expr, item), {})

    def convert_get(self, context, expr, item, default):
        if item is None or expr is None:
            return None

        item = item.convert_to_type(self.keyType, ConversionLevel.UpcastContainers)
        if item is None:
            return None

        return context.call_py_function(dict_get, (expr, item, default), {})

    def convert_setitem(self, context, expr, key, value):
        if key is None or expr is None or value is None:
            return None

        key = key.convert_to_type(self.keyType, ConversionLevel.UpcastContainers)
        if key is None:
            return None

        value = value.convert_to_type(self.valueType, ConversionLevel.ImplicitContainers)
        if value is None:
            return None

        return context.call_py_function(dict_setitem, (expr, key, value), {})

    def convert_items_reserved_native(self, expr):
        if isinstance(expr, TypedExpression):
            expr = expr.nonref_expr
        return expr.ElementPtrIntegers(0, 3).load().cast(native_ast.Int64)

    def convert_items_reserved(self, context, expr):
        return context.pushPod(int, self.convert_items_reserved_native(expr))

    def convert_slot_populated_native(self, expr, slotIx):
        if isinstance(expr, TypedExpression):
            expr = expr.nonref_expr
        return expr.ElementPtrIntegers(0, 2).load().elemPtr(slotIx.nonref_expr).load()

    def convert_len(self, context, expr):
        return context.pushPod(int, self.convert_len_native(expr))

    def convert_bin_op_reverse(self, context, left, op, right, inplace):
        if op.matches.In:
            right = right.convert_to_type(self.keyType, ConversionLevel.UpcastContainers)
            if right is None:
                return None

            return context.call_py_function(
                dict_table_contains,
                (left, right),
                {}
            )

        return super().convert_bin_op_reverse(context, left, op, right, inplace)

    def convert_getkey_by_index_unsafe(self, context, expr, item):
        return context.pushReference(
            self.keyType,
            expr.nonref_expr.ElementPtrIntegers(0, 1).load()
                .elemPtr(item.nonref_expr.mul(native_ast.const_int_expr(self.kvBytecount)))
                .cast(self.keyType.getNativeLayoutType().pointer())
        )

    def convert_getvalue_by_index_unsafe(self, context, expr, item):
        return context.pushReference(
            self.valueType,
            expr.nonref_expr.ElementPtrIntegers(0, 1).load()
            .elemPtr(
                item.nonref_expr.mul(native_ast.const_int_expr(self.kvBytecount))
                .add(native_ast.const_int_expr(self.keyBytecount))
            ).cast(self.valueType.getNativeLayoutType().pointer())
        )

    def generateNativeDestructorFunction(self, context, out, inst):
        with context.loop(self.convert_items_reserved(context, inst)) as i:
            with context.ifelse(self.convert_slot_populated_native(inst, i).neq(0)) as (then, otherwise):
                with then:
                    self.convert_getkey_by_index_unsafe(context, inst, i).convert_destroy()
                    self.convert_getvalue_by_index_unsafe(context, inst, i).convert_destroy()

        context.pushEffect(
            runtime_functions.free.call(inst.nonref_expr.ElementPtrIntegers(0, 1).load().cast(native_ast.UInt8Ptr)) >>
            runtime_functions.free.call(inst.nonref_expr.ElementPtrIntegers(0, 2).load().cast(native_ast.UInt8Ptr)) >>
            runtime_functions.free.call(inst.nonref_expr.ElementPtrIntegers(0, 5).load().cast(native_ast.UInt8Ptr)) >>
            runtime_functions.free.call(inst.nonref_expr.ElementPtrIntegers(0, 6).load().cast(native_ast.UInt8Ptr)) >>
            runtime_functions.free.call(inst.nonref_expr.cast(native_ast.UInt8Ptr))
        )

    def convert_type_call(self, context, typeInst, args, kwargs):
        if len(args) == 0 and not kwargs:
            return context.push(self, lambda x: x.convert_default_initialize())

        if len(args) == 1 and not kwargs:
            if args[0].expr_type == self:
                return context.call_py_function(dict_duplicate, (args[0],), {})
            else:
                return args[0].convert_to_type(self, ConversionLevel.New)

        return super().convert_type_call(context, typeInst, args, kwargs)

    def _can_convert_to_type(self, targetType, conversionLevel):
        if not conversionLevel.isNewOrHigher():
            return False

        if targetType.typeRepresentation is bool:
            return True

        if targetType.typeRepresentation is str:
            return "Maybe"

        return False

    def convert_to_type_with_target(self, context, instance, targetVal, conversionLevel, mayThrowOnFailure=False):
        if targetVal.expr_type.typeRepresentation is bool:
            res = context.pushPod(bool, self.convert_len_native(instance.nonref_expr).neq(0))
            context.pushEffect(
                targetVal.expr.store(res.nonref_expr)
            )
            return context.constant(True)

        return super().convert_to_type_with_target(context, instance, targetVal, conversionLevel, mayThrowOnFailure)


@TypeFunction
def DictKeysIterator(K, V):
    @Held
    class DictKeysIterator(
        Class,
        Final,
        __name__=f"DictKeysIterator({K.__name__}, {V.__name__})"
    ):
        slotIx = Member(int, nonempty=True)
        count = Member(int, nonempty=True)
        instance = Member(Dict(K, V), nonempty=True)

        def __init__(self, instance):
            self.instance = instance
            self.slotIx = -1
            self.count = len(instance)

        def __iter__(self):
            return self

        def __next__(self) -> K:
            res = self.__fastnext__()
            if res:
                return res.get()
            raise StopIteration()

        def __fastnext__(self) -> PointerTo(K):
            checkDictSizeAndThrowIfChanged(self.instance, self.count)

            nextSlotIx = table_next_slot(self.instance, self.slotIx)

            if nextSlotIx >= 0:
                self.slotIx = nextSlotIx
                return self.instance.getKeyPtrByIndexUnsafe(nextSlotIx)
            else:
                return PointerTo(K)()

    return DictKeysIterator


@TypeFunction
def DictValuesIterator(K, V):
    @Held
    class DictValuesIterator(
        Class,
        Final,
        __name__=f"DictValuesIterator({K.__name__}, {V.__name__})"
    ):
        slotIx = Member(int, nonempty=True)
        count = Member(int, nonempty=True)
        instance = Member(Dict(K, V), nonempty=True)

        def __init__(self, instance):
            self.instance = instance
            self.slotIx = -1
            self.count = len(instance)

        def __iter__(self):
            return self

        def __next__(self) -> V:
            res = self.__fastnext__()
            if res:
                return res.get()
            raise StopIteration()

        def __fastnext__(self) -> PointerTo(V):
            checkDictSizeAndThrowIfChanged(self.instance, self.count)

            nextSlotIx = table_next_slot(self.instance, self.slotIx)

            if nextSlotIx >= 0:
                self.slotIx = nextSlotIx
                return self.instance.getValuePtrByIndexUnsafe(nextSlotIx)
            else:
                return PointerTo(V)()

    return DictValuesIterator


@TypeFunction
def DictItemsIterator(K, V):
    @Held
    class DictItemsIterator(
        Class,
        Final,
        __name__=f"DictItemsIterator({K.__name__}, {V.__name__})"
    ):
        slotIx = Member(int, nonempty=True)
        count = Member(int, nonempty=True)
        instance = Member(Dict(K, V), nonempty=True)

        def __init__(self, instance):
            self.instance = instance
            self.slotIx = -1
            self.count = len(instance)

        def __iter__(self):
            return self

        def __next__(self) -> Tuple(K, V):
            res = self.__fastnext__()
            if res:
                return res.get()
            raise StopIteration()

        def __fastnext__(self) -> PointerTo(Tuple(K, V)):
            checkDictSizeAndThrowIfChanged(self.instance, self.count)

            nextSlotIx = table_next_slot(self.instance, self.slotIx)

            if nextSlotIx >= 0:
                self.slotIx = nextSlotIx
                return self.instance.getItemPtrByIndexUnsafe(nextSlotIx)
            else:
                return PointerTo(Tuple(K, V))()

    return DictItemsIterator
