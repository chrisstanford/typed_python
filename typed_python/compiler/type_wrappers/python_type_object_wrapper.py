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

import typed_python.compiler
from typed_python import Type
from typed_python.compiler.conversion_level import ConversionLevel
from typed_python.compiler.type_wrappers.wrapper import Wrapper
from typed_python.compiler.type_wrappers.python_free_object_wrapper import PythonFreeObjectWrapper
from typed_python.compiler.type_wrappers.compilable_builtin import CompilableBuiltin

typeWrapper = lambda t: typed_python.compiler.python_object_representation.typedPythonTypeToTypeWrapper(t)


class PythonTypeObjectWrapper(PythonFreeObjectWrapper):
    def __init__(self, f):
        super().__init__(f, hasSideEffects=False)

    def __repr__(self):
        return "Wrapper(TypeObject(%s))" % self.typeRepresentation.Value.__qualname__

    def __str__(self):
        return "TypeObject(%s)" % self.typeRepresentation.Value.__qualname__

    @Wrapper.unwrapOneOfAndValue
    def convert_call_on_container_expression(self, context, inst, argExpr):
        if issubclass(self.typeRepresentation.Value, CompilableBuiltin):
            return self.typeRepresentation.Value.convert_type_call_on_container_expression(context, inst, argExpr)

        return typeWrapper(self.typeRepresentation.Value).convert_type_call_on_container_expression(context, inst, argExpr)

    def convert_attribute(self, context, instance, attribute, allowDefer=True):
        if allowDefer:
            return typeWrapper(self.typeRepresentation.Value).convert_type_attribute(context, instance, attribute)
        else:
            return super().convert_attribute(context, instance, attribute)

    def convert_method_call(self, context, instance, methodname, args, kwargs, allowDefer=True):
        if allowDefer:
            return typeWrapper(self.typeRepresentation.Value).convert_type_method_call(context, instance, methodname, args, kwargs)
        else:
            return super().convert_method_call(context, instance, methodname, args, kwargs)

    @Wrapper.unwrapOneOfAndValue
    def convert_call(self, context, left, args, kwargs):
        if self.typeRepresentation.Value is type:
            # make sure we don't have any masquerades in here
            if len(args) != 1 or kwargs:
                return super().convert_call(context, left, args, kwargs)

            argtype = args[0].expr_type

            if isinstance(argtype, PythonTypeObjectWrapper):
                res = typed_python.compiler.python_object_representation.pythonObjectRepresentation(
                    context,
                    type
                )
            else:
                res = typed_python.compiler.python_object_representation.pythonObjectRepresentation(
                    context,
                    argtype.interpreterTypeRepresentation
                )

            return res

        if len(args) == 1 and not kwargs:
            if self.typeRepresentation.Value in (bool, int, float, str, bytes):
                return args[0].convert_to_type(self.typeRepresentation.Value, ConversionLevel.New)

        if Type in self.typeRepresentation.Value.__bases__:
            # this is one of the type factories (ListOf, Dict, etc.)
            return super().convert_call(context, left, args, kwargs)

        if issubclass(self.typeRepresentation.Value, CompilableBuiltin):
            return self.typeRepresentation.Value.convert_type_call(context, left, args, kwargs)

        return typeWrapper(self.typeRepresentation.Value).convert_type_call(context, left, args, kwargs)
