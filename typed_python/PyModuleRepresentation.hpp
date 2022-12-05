/******************************************************************************
   Copyright 2017-2021 typed_python Authors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
******************************************************************************/

#pragma once

#include "ModuleRepresentation.hpp"
#include <memory>

class PyModuleRepresentation {
public:
    PyObject_HEAD

    std::shared_ptr<ModuleRepresentation> mModuleRepresentation;

    static void dealloc(PyModuleRepresentation *self);

    static PyObject *new_(PyTypeObject *type, PyObject *args, PyObject *kwargs);

    static int init(PyModuleRepresentation *self, PyObject *args, PyObject *kwargs);

    static PyObject* addExternal(PyModuleRepresentation* self, PyObject* args, PyObject* kwargs);

    static PyObject* getDict(PyModuleRepresentation* self, PyObject* args, PyObject* kwargs);

    static PyObject* isSetupComplete(PyModuleRepresentation* self, PyObject* args, PyObject* kwargs);

    static PyObject* update(PyModuleRepresentation* self, PyObject* args, PyObject* kwargs);

    static PyObject* setupComplete(PyModuleRepresentation* self, PyObject* args, PyObject* kwargs);

    static PyObject* copyInto(PyModuleRepresentation* self, PyObject* args, PyObject* kwargs);

    static PyObject* copyIntoAsInactive(PyModuleRepresentation* self, PyObject* args, PyObject* kwargs);

    static PyObject* oidFor(PyModuleRepresentation* self, PyObject* args, PyObject* kwargs);
};

extern PyTypeObject PyType_ModuleRepresentation;
