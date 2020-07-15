#   Copyright 2017-2019 typed_python Authors
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

from typed_python import (
    TypeFunction, Class, Alternative, Member, SerializationContext,
    Forward, ListOf, Final, isCompiled, Entrypoint, NotCompiled
)
from typed_python.compiler.runtime import Runtime

import unittest


@TypeFunction
def TfLevelMethod(T):
    @Entrypoint
    def aMethod2(x: int) -> int:
        return x

    @Entrypoint
    def aMethod(x) -> int:
        return aMethod2(x)

    class A(Class, Final):
        def f(self, x):
            return aMethod(x)

    return A


class TypeFunctionTest(unittest.TestCase):
    def test_basic(self):
        @TypeFunction
        def List(T):
            return Alternative(
                "List",
                Node={"head": T, "tail": List(T)},
                Empty={}
            )

        self.assertIs(List(int), List(int))
        self.assertIsNot(List(float), List(int))

        l_i = List(int).Empty()
        l_f = List(float).Empty()

        List(int).Node(head=10, tail=l_i)
        List(float).Node(head=10.5, tail=l_f)

        with self.assertRaises(TypeError):
            List(int).Node(head=10, tail=l_f)

    def test_mutually_recursive(self):
        @TypeFunction
        def X(T):
            class X(Class):
                i = Member(T)
                y = Member(Y(T))
            return X

        @TypeFunction
        def Y(T):
            class Y(Class):
                i = Member(T)
                x = Member(X(T))
            return Y

        self.assertIs(Y(int), Y(int))
        self.assertIs(Y(int).x.type, X(int))
        self.assertIs(Y(int).x.type.y.type, Y(int))

        anX = X(int)()
        anX.y = Y(int)()
        anX.y.x = anX

        with self.assertRaises(TypeError):
            anX.y = Y(float)()

    def test_can_serialize_type_functions(self):
        @TypeFunction
        def List(T):
            ListT = Forward("ListOf(T)")
            return ListT.define(Alternative(
                "ListOf(T)",
                Node={"head": T, "tail": ListT},
                Empty={}
            ))

        context = SerializationContext({'List': List})

        self.assertIs(
            context.deserialize(context.serialize(List(int))),
            List(int)
        )
        self.assertIsInstance(
            context.deserialize(context.serialize(List(int).Empty())),
            List(int)
        )

        list_of_int = List(int)
        list_of_list = List(list_of_int)

        l0 = list_of_int.Empty()
        l_l = list_of_list.Node(head=l0, tail=list_of_list.Empty())

        self.assertEqual(
            context.deserialize(context.serialize(l_l)),
            l_l
        )

    def test_type_function_on_typed_lists(self):
        @TypeFunction
        def SumThem(x):
            summedValues = sum(x)

            class C:
                X = summedValues

            return C

        self.assertIs(
            SumThem([1, 2, 3]),
            SumThem(ListOf(int)([1, 2, 3]))
        )

        self.assertEqual(SumThem(ListOf(int)([1, 2, 3])).X, 6)

    def test_type_functions_with_recursive_annotations(self):
        @TypeFunction
        def Boo(T):
            class Boo_(Class, Final):
                def f(self) -> Boo(T):
                    return self
            return Boo_

        boo = Boo(int)()

        self.assertEqual(boo, boo.f())

    def test_compile_typefunc_staticmethod(self):
        @TypeFunction
        def A(T):
            class MyClass(Class):
                @Entrypoint
                @staticmethod
                def aFunc(x: ListOf(T)) -> ListOf(T):
                    # accessing 'T' means that this is now a
                    # closure, holding 'T' in a 'global cell'
                    T

                    assert isCompiled()

                    return x

            return MyClass

        @Entrypoint
        def callAFunc():
            return A(float).aFunc(ListOf(float)([1, 2]))

        # we should be able to dispatch to this from the compiler
        self.assertEqual(callAFunc(), [1, 2])

        # we should also know that this is an entrypoint
        self.assertTrue(A(float).aFunc.isEntrypoint)

        # we should be able to dispatch to this from the interpreter and not
        # trip the assertion on isCompiled
        self.assertEqual(A(float).aFunc(ListOf(float)([1, 2])), [1, 2])

    def test_typefunc_in_staticmethod_annotation(self):
        @TypeFunction
        def makeClass(T):
            class A(Class):
                @staticmethod
                def f() -> makeClass(T):
                    return makeClass(T)()
            return A

        makeClass(int)()

    def test_typefunc_in_staticmethod_annotation_notcompiled(self):
        @TypeFunction
        def makeClass(T):
            class A(Class):
                @staticmethod
                @NotCompiled
                def f() -> makeClass(T):
                    return makeClass(T)()
            return A

        makeClass(int)()

    def test_compiled_method_in_tf_closure(self):
        timesCompiled = Runtime.singleton().timesCompiled

        for _ in range(1000):
            TfLevelMethod(str)().f(1)

        assert Runtime.singleton().timesCompiled - timesCompiled < 10

    def test_pass_function_with_reference_doesnt_recompile(self):
        timesCompiled = Runtime.singleton().timesCompiled

        @NotCompiled
        def f(x):
            return x + 1

        def g(x):
            return f(x)

        @Entrypoint
        def callIt(aFun, anArg):
            return aFun(anArg)

        for _ in range(1000):
            callIt(g, 10)

        assert Runtime.singleton().timesCompiled - timesCompiled < 10

    def test_module_level_type_function_name(self):
        assert SerializationContext().nameForObject(TfLevelMethod) is not None

    def test_deserialize_type_functions_usable(self):
        @TypeFunction
        def TF(T):
            class TF(Class):
                x = Member(T)

            return TF

        TF2 = SerializationContext().deserialize(SerializationContext().serialize(TF))

        assert TF2(int)(x=20).x == 20

        TFInt = SerializationContext().deserialize(SerializationContext().serialize(TF(int)))

        assert TFInt(x=20).x == 20

    def test_classes_binding_methods_with_closures(self):
        def makeF(boundvalue):
            def f():
                return boundvalue
            return f

        def makeFClass(x):
            f = makeF(x)

            class T(Class, Final):
                def classF(self):
                    return f()

            return T

        @Entrypoint
        def callIt(T):
            return T().classF()

        self.assertEqual(callIt(makeFClass(10)), 10)
        self.assertEqual(callIt(makeFClass(20)), 20)
        self.assertEqual(callIt(makeFClass((1, 2))), (1, 2))
        self.assertEqual(callIt(makeFClass((1, 3))), (1, 3))
