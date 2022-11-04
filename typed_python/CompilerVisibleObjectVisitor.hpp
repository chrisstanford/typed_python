/******************************************************************************
   Copyright 2017-2022 typed_python Authors

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

#include "ShaHash.hpp"
#include "util.hpp"
#include <unordered_map>

/******************************

This module provides services for walking the python object and Type object graph
with the same level of detail that the compiler does.  We use this to build a
unique hash for types and functions.

******************************/

bool isCanonicalName(std::string name) {
    // this is the list of standard library modules in python 3.8
    static std::set<std::string> canonicalPythonModuleNames({
        "abc", "aifc", "antigravity", "argparse", "ast", "asynchat", "asyncio", "asyncore",
        "base64", "bdb", "binhex", "bisect", "_bootlocale", "bz2", "calendar", "cgi", "cgitb",
        "chunk", "cmd", "codecs", "codeop", "code", "collections", "_collections_abc",
        "colorsys", "_compat_pickle", "compileall", "_compression", "concurrent",
        "configparser", "contextlib", "contextvars", "copy", "copyreg", "cProfile", "crypt",
        "csv", "ctypes", "curses", "dataclasses", "datetime", "dbm", "decimal", "difflib",
        "dis", "distutils", "doctest", "dummy_threading", "_dummy_thread", "email",
        "encodings", "ensurepip", "enum", "filecmp", "fileinput", "fnmatch", "formatter",
        "fractions", "ftplib", "functools", "__future__", "genericpath", "getopt", "getpass",
        "gettext", "glob", "gzip", "hashlib", "heapq", "hmac", "html", "http", "idlelib",
        "imaplib", "imghdr", "importlib", "imp", "inspect", "io", "ipaddress", "json",
        "keyword", "lib2to3", "linecache", "locale", "logging", "lzma", "mailbox", "mailcap",
        "marshal",
        "_markupbase", "mimetypes", "modulefinder", "msilib", "multiprocessing", "netrc",
        "nntplib", "ntpath", "nturl2path", "numbers", "opcode", "operator", "optparse", "os",
        "_osx_support", "pathlib", "pdb", "pickle", "pickletools", "pipes", "pkgutil",
        "platform", "plistlib", "poplib", "posixpath", "pprint", "profile", "pstats", "pty",
        "_py_abc", "pyclbr", "py_compile", "_pydecimal", "pydoc_data", "pydoc", "_pyio",
        "queue", "quopri", "random", "reprlib", "re", "rlcompleter", "runpy", "sched",
        "secrets", "selectors", "shelve", "shlex", "shutil", "signal", "_sitebuiltins",
        "site-packages", "site", "smtpd", "smtplib", "sndhdr", "socket", "socketserver",
        "sqlite3", "sre_compile", "sre_constants", "sre_parse", "ssl", "statistics", "stat",
        "stringprep", "string", "_strptime", "struct", "subprocess", "sunau", "symbol",
        "symtable", "sysconfig", "tabnanny", "tarfile", "telnetlib", "tempfile", "test",
        "textwrap", "this", "_threading_local", "threading", "timeit", "tkinter", "tokenize",
        "token", "traceback", "tracemalloc", "trace", "tty", "turtledemo", "turtle", "types",
        "typing", "unittest", "urllib", "uuid", "uu", "venv", "warnings", "wave", "weakref",
        "_weakrefset", "webbrowser", "wsgiref", "xdrlib", "xml", "xmlrpc", "zipapp",
        "zipfile", "zipimport", "pytz", "psutil",

        // and some standard ones we might commonly install
        "numpy", "pandas", "scipy", "pytest", "_pytest", "typed_python", "object_database", "llvmlite",
        "requests", "redis", "websockets", "boto3", "py", "xdist", "pytest_jsonreport",
        "pytest_metadata", "flask", "flaky", "coverage", "pyasn1", "cryptography", "paramiko",
        "six", "torch"
    });

    std::string moduleNameRoot;

    int posOfDot = name.find(".");
    if (posOfDot != std::string::npos) {
        moduleNameRoot = name.substr(0, posOfDot);
    } else {
        moduleNameRoot = name;
    }

    return canonicalPythonModuleNames.find(moduleNameRoot) != canonicalPythonModuleNames.end();
}

// is this a special name in a dict, module, or class that we shouldn't hash?
// we do want to hash methods like __init__
bool isSpecialIgnorableName(const std::string& name) {
    static std::set<std::string> canonicalMagicMethods({
        "__abs__", "__add__", "__and__", "__bool__",
        "__bytes__", "__call__", "__contains__", "__del__",
        "__delattr__", "__eq__", "__float__", "__floordiv__",
        "__format__", "__ge__", "__getitem__", "__gt__",
        "__hash__", "__iadd__", "__iand__", "__ieq__",
        "__ifloordiv__", "__ige__", "__igt__", "__ile__",
        "__ilshift__", "__ilt__", "__imatmul__", "__imod__",
        "__imul__", "__index__", "__ine__", "__init__",
        "__int__", "__invert__", "__ior__", "__ipow__",
        "__irshift__", "__isub__", "__itruediv__", "__ixor__",
        "__le__", "__len__", "__lshift__", "__lt__",
        "__matmul__", "__mod__", "__mul__", "__ne__",
        "__neg__", "__not__", "__or__", "__pos__",
        "__pow__", "__radd__", "__rand__", "__repr__",
        "__rfloordiv__", "__rlshift__", "__rmatmul__", "__rmod__",
        "__rmul__", "__ror__", "__round__", "__round__",
        "__rpow__", "__rrshift__", "__rshift__", "__rsub__",
        "__rtruediv__", "__rxor__", "__setattr__", "__setitem__",
        "__str__", "__sub__", "__truediv__", "__xor__",
    });

    return (
        name.substr(0, 2) == "__"
        && name.substr(name.size() - 2) == "__"
        && canonicalMagicMethods.find(name) == canonicalMagicMethods.end()
    );
}


class VisitRecord {
public:
    enum class kind { Hash=0, String=1, Topo=2, NameValuePair=3, Error=4 };

    VisitRecord() : mKind(kind::Error) {}

    VisitRecord(ShaHash hash) :
        mHash(hash),
        mKind(kind::Hash)
    {}

    VisitRecord(std::string name) :
        mName(name), mKind(kind::String)
    {}

    VisitRecord(std::string name, TypeOrPyobj topo)
        : mName(name), mTopo(topo), mKind(kind::NameValuePair)
    {}

    VisitRecord(TypeOrPyobj topo)
        : mTopo(topo), mKind(kind::Topo)
    {}

    static VisitRecord Err(std::string err) {
        VisitRecord res;
        res.mErr = err;
        return res;
    }

    bool operator==(const VisitRecord& other) const {
        if (mKind != other.mKind) {
            return false;
        }

        if (mKind == kind::Hash) {
            return mHash == other.mHash;
        }

        if (mKind == kind::Topo) {
            return mTopo == other.mTopo;
        }

        if (mKind == kind::String) {
            return mName == other.mName;
        }

        if (mKind == kind::NameValuePair) {
            return mName == other.mName && mTopo == other.mTopo;
        }

        if (mKind == kind::Error) {
            return mErr == other.mErr;
        }

        return true;
    }

    std::string err() const {
        return mErr;
    }

    std::string name() const {
        return mName;
    }

    TypeOrPyobj topo() const {
        return mTopo;
    }

    ShaHash hash() const {
        return mHash;
    }

    kind getKind() const {
        return mKind;
    }

    std::string toString() const {
        if (mKind == kind::Error) {
            return "Err(" + mErr + ")";
        }

        if (mKind == kind::String) {
            return "String(" + mName + ")";
        }
        if (mKind == kind::Hash) {
            return "Hash(" + mHash.digestAsHexString() + ")";
        }
        if (mKind == kind::Topo) {
            return "Topo(" + mTopo.name() + ")";
        }
        if (mKind == kind::NameValuePair) {
            return "NameValuePair(" + mName + "=" + mTopo.name() + ")";
        }

        return "<Unknown>";
    }

private:
    kind mKind;
    std::string mName;
    std::string mErr;
    ShaHash mHash;
    TypeOrPyobj mTopo;
};


template<class visitor_1, class visitor_2, class visitor_3, class visitor_4, class visitor_5>
class LambdaVisitor {
public:
    LambdaVisitor(
        const visitor_1& hashVisit,
        const visitor_2& nameVisit,
        const visitor_3& topoVisitor,
        const visitor_4& namedVisitor,
        const visitor_5& onErr
    ):
        mHashVisit(hashVisit),
        mNameVisit(nameVisit),
        mTopoVisitor(topoVisitor),
        mNamedVisitor(namedVisitor),
        mOnErr(onErr)
    {}

    void visitHash(ShaHash h) const {
        mHashVisit(h);
    }

    void visitName(std::string name) const {
        mNameVisit(name);
    }

    void visitTopo(TypeOrPyobj topo) const {
        mTopoVisitor(topo);
    }

    void visitNamedTopo(std::string name, TypeOrPyobj instance) const {
        mNamedVisitor(name, instance);
    }

    void visitErr(std::string err) const {
        mOnErr(err);
    }

private:
    const visitor_1& mHashVisit;
    const visitor_2& mNameVisit;
    const visitor_3& mTopoVisitor;
    const visitor_4& mNamedVisitor;
    const visitor_5& mOnErr;
};




class CompilerVisibleObjectVisitor {
public:
    static CompilerVisibleObjectVisitor& singleton() {
        static CompilerVisibleObjectVisitor* visitor = new CompilerVisibleObjectVisitor();

        return *visitor;
    }

    /*******
        This function defines  generic visitor pattern for looking inside of a Type or a PyObject to see
          which pieces of are visible to the compiler. We try to hold this all in one place so that we can
          have a single well-defined semantic for how we're visiting and hashing our objects.

        Our general rule is that objects visible at module level scope will never have their identities
        reassigned, nor will regular class members be reassigned. However, mutable containers may change.

        This function accepts a set of template parameters that get called with the internal pieces of the
        object:
            hashVisit(ShaHash): used to visit a single hash-hash
            nameVisit(string): used to visit a string (say, the name of a function)
            topoVisitor(TypeOrPyobj): looks at the actual instances
            namedVisitor(string, TypeOrPyobj): looks at (name, TypeOrPyobj) pairs (for walking dicts)
            tpInstanceVisitor(Instance): look at visible instances
            onErr(): gets called if something odd happens (missing or badly typed member)
    ********/
    template<class visitor_1, class visitor_2, class visitor_3, class visitor_4, class visitor_5>
    void visit(
        TypeOrPyobj obj,
        const visitor_1& hashVisit,
        const visitor_2& nameVisit,
        const visitor_3& topoVisitor,
        const visitor_4& namedVisitor,
        const visitor_5& onErr
    ) {
        visit(
            obj,
            LambdaVisitor<visitor_1, visitor_2, visitor_3, visitor_4, visitor_5>(
                hashVisit, nameVisit, topoVisitor, namedVisitor, onErr
            )
        );
    }

    template<class visitor_type>
    void visit(
        TypeOrPyobj obj,
        const visitor_type& visitor
    ) {
        std::vector<VisitRecord> records = recordWalk(obj);

        auto it = mPastVisits.find(obj);
        if (it == mPastVisits.end()) {
            mPastVisits[obj] = records;
        } else {
            if (it->second != records) {
                checkForInstability();

                throw std::runtime_error(
                    "Found unstable object, but somehow our instability check"
                    " didn't throw an exception?" + obj.name()
                );
            }
        }

        walk(obj, visitor);
    }

    static std::string recordWalkAsString(TypeOrPyobj obj) {
        std::ostringstream s;

        for (auto& record: recordWalk(obj)) {
            s << record.toString() << "\n";
        }

        return s.str();
    }

    static std::vector<VisitRecord> recordWalk(TypeOrPyobj obj) {
        std::vector<VisitRecord> records;

        walk(
            obj,
            [&](ShaHash h) { records.push_back(VisitRecord(h)); },
            [&](std::string h) { records.push_back(VisitRecord(h)); },
            [&](TypeOrPyobj o) { records.push_back(VisitRecord(o)); },
            [&](std::string n, TypeOrPyobj o) { records.push_back(VisitRecord(n, o)); },
            [&](std::string err) { records.push_back(VisitRecord::Err(err)); }
        );

        return records;
    }

    void resetCache() {
        mPastVisits.clear();
    }

    void checkForInstability() {
        std::vector<TypeOrPyobj> unstable;

        for (auto it = mPastVisits.begin(); it != mPastVisits.end(); ++it) {
            if (it->second != recordWalk(it->first)) {
                unstable.push_back(it->first);
            }
        }

        if (!unstable.size()) {
            return;
        }

        std::ostringstream s;

        s << "Found " << unstable.size() << " unstable objects\n";

        for (long k = 0; k < unstable.size() && k < 1000; k++) {
            s << k << " -> " << unstable[k].name() << "\n";

            std::vector<std::string> linesLeft = stringifyVisitRecord(recordWalk(unstable[k]));
            std::vector<std::string> linesRight = stringifyVisitRecord(mPastVisits[unstable[k]]);

            auto pad = [&](std::string s, int ct) {
                if (s.size() > ct) {
                    return s.substr(0, ct);
                }

                return s + std::string(ct - s.size(), ' ');
            };

            for (long j = 0; j < linesLeft.size() || j < linesRight.size(); j++) {
                s << "    ";

                if (j < linesLeft.size()) {
                    s << pad(linesLeft[j], 80);
                } else {
                    s << pad("", 80);
                }

                s << "   |   ";

                if (j < linesRight.size()) {
                    s << pad(linesRight[j], 80);
                } else {
                    s << pad("", 80);
                }

                s << "\n";
            }
        }

        throw std::runtime_error(s.str());
    }

    std::vector<std::string> stringifyVisitRecord(const std::vector<VisitRecord>& records) {
        std::vector<std::string> lines;

        for (auto& record: records) {
            lines.push_back(record.toString());
        }

        return lines;
    }

    // is this a 'globally identifiable' py object, where we can just use its name to find it,
    // and where we are guaranteed that it won't change between invocations of the program?
    static bool isPyObjectGloballyIdentifiableAndStable(PyObject* h) {
        if (isPyObjectGloballyIdentifiable(h)) {
            std::string moduleName = std::string(PyUnicode_AsUTF8(PyObject_GetAttrString(h, "__module__")));
            std::string clsName = std::string(PyUnicode_AsUTF8(PyObject_GetAttrString(h, "__name__")));

            if (isCanonicalName(moduleName) || h->ob_type == &PyCFunction_Type) {
                return true;
            }
        }

        return false;
    }

    // is this a 'globally identifiable' py object, where we can just use its name.
    static bool isPyObjectGloballyIdentifiable(PyObject* h) {
        PyEnsureGilAcquired getTheGil;

        static PyObject* sysModule = PyImport_ImportModule("sys");
        static PyObject* sysModuleModules = PyObject_GetAttrString(sysModule, "modules");

        if (PyObject_HasAttrString(h, "__module__") && PyObject_HasAttrString(h, "__name__")) {
            PyObjectStealer moduleName(PyObject_GetAttrString(h, "__module__"));
            if (!moduleName) {
                PyErr_Clear();
                return false;
            }

            PyObjectStealer clsName(PyObject_GetAttrString(h, "__name__"));
            if (!clsName) {
                PyErr_Clear();
                return false;
            }

            if (!PyUnicode_Check(moduleName) || !PyUnicode_Check(clsName)) {
                return false;
            }

            PyObjectStealer moduleObject(PyObject_GetItem(sysModuleModules, moduleName));

            if (!moduleObject) {
                PyErr_Clear();
                return false;
            }

            PyObjectStealer obj(PyObject_GetAttr(moduleObject, clsName));

            if (!obj) {
                PyErr_Clear();
                return false;
            }
            if ((PyObject*)obj == h) {
                return true;
            }
        }

        return false;
    }

    // is this a 'simple' py object which we wouldn't want to step into?
    static bool isSimpleConstant(PyObject* h) {
        // TODO: can we figure out how to do this without holding the GIL? None of the objects
        // in question should get deleted ever...
        PyEnsureGilAcquired getTheGil;

        static PyObject* builtinsModule = ::builtinsModule();
        static PyObject* builtinsModuleDict = PyObject_GetAttrString(builtinsModule, "__dict__");

        // handle basic constants
        return (
               h == Py_None
            || h == Py_True
            || h == Py_False
            || PyLong_Check(h)
            || PyBytes_Check(h)
            || PyUnicode_Check(h)
            || h == builtinsModule
            || h == builtinsModuleDict
            || h == (PyObject*)PyDict_Type.tp_base
            || h == (PyObject*)&PyType_Type
            || h == (PyObject*)&PyDict_Type
            || h == (PyObject*)&PyList_Type
            || h == (PyObject*)&PySet_Type
            || h == (PyObject*)&PyLong_Type
            || h == (PyObject*)&PyUnicode_Type
            || h == (PyObject*)&PyFloat_Type
            || h == (PyObject*)&PyBytes_Type
            || h == (PyObject*)&PyBool_Type
            || h == (PyObject*)Py_None->ob_type
            || h == (PyObject*)&PyProperty_Type
            || h == (PyObject*)&PyClassMethodDescr_Type
            || h == (PyObject*)&PyGetSetDescr_Type
            || h == (PyObject*)&PyMemberDescr_Type
            || h == (PyObject*)&PyMethodDescr_Type
            || h == (PyObject*)&PyWrapperDescr_Type
            || h == (PyObject*)&PyDictProxy_Type
            || h == (PyObject*)&_PyMethodWrapper_Type
            || h == (PyObject*)&PyCFunction_Type
            || h == (PyObject*)&PyFunction_Type
            || PyFloat_Check(h)
            || h->ob_type == &PyProperty_Type
            || h->ob_type == &PyGetSetDescr_Type
            || h->ob_type == &PyMemberDescr_Type
            || h->ob_type == &PyWrapperDescr_Type
            || h->ob_type == &PyDictProxy_Type
            || h->ob_type == &_PyMethodWrapper_Type
        );
    }

private:
    template<class visitor_1, class visitor_2, class visitor_3, class visitor_4, class visitor_5>
    static void walk(
        TypeOrPyobj obj,
        const visitor_1& hashVisit,
        const visitor_2& nameVisit,
        const visitor_3& topoVisitor,
        const visitor_4& namedVisitor,
        const visitor_5& onErr
    ) {
        walk(obj,
            LambdaVisitor<visitor_1, visitor_2, visitor_3, visitor_4, visitor_5>(
                hashVisit, nameVisit, topoVisitor, namedVisitor, onErr
            )
        );
    }

    template<class visitor_type>
    static void walk(
        TypeOrPyobj obj,
        const visitor_type& visitor
    ) {
        PyEnsureGilAcquired getTheGil;

        auto visitDict = [&](PyObject* d, bool ignoreSpecialNames=false) {
            if (!d) {
                visitor.visitHash(0);
                return;
            }

            if (!PyDict_Check(d)) {
                visitor.visitErr(std::string("not a dict: ") + d->ob_type->tp_name);
                return;
            }

            // get a list of the names in order. We have to walk them
            // in lexical order to make sure that our hash is stable.
            std::map<std::string, PyObject*> names;
            iterate(d, [&](PyObject* o) {
                if (PyUnicode_Check(o)) {
                    std::string name = PyUnicode_AsUTF8(o);

                    // we don't want module members to hash their file paths
                    // or their module loader info, because then they can't be
                    // moved around without violating the cache (and in fact their
                    // hashes are not stable at all)
                    if (!(ignoreSpecialNames && isSpecialIgnorableName(name))) {
                        names[name] = o;
                    }
                }
            });

            visitor.visitHash(ShaHash(names.size()));

            for (auto nameAndO: names) {
                PyObject* val = PyDict_GetItem(d, nameAndO.second);
                if (!val) {
                    PyErr_Clear();
                    visitor.visitErr("dict getitem empty");
                } else {
                    visitor.visitNamedTopo(nameAndO.first, val);
                }
            }
        };

        auto visitTuple = [&](PyObject* t) {
            if (!t) {
                visitor.visitHash(ShaHash(0));
                return;
            }

            visitor.visitHash(ShaHash(PyTuple_Size(t)));
            for (long k = 0; k < PyTuple_Size(t); k++) {
                visitor.visitTopo(PyTuple_GetItem(t, k));
            }
        };

        auto visitDictOrTuple = [&](PyObject* t) {
            if (!t) {
                visitor.visitHash(ShaHash(0));
                return;
            }

            if (PyDict_Check(t)) {
                visitDict(t);
                return;
            }

            if (PyTuple_Check(t)) {
                visitTuple(t);
                return;
            }

            visitor.visitErr("not a dict or tuple");
        };

        if (obj.type()) {
            Type* objType = obj.type();

            visitor.visitHash(ShaHash(1));

            objType->visitCompilerVisibleInternals(visitor);

            return;
        }

        static PyObject* osModule = ::osModule();
        static PyObject* environType = PyObject_GetAttrString(osModule, "_Environ");

        if (obj.pyobj()->ob_type == (PyTypeObject*)environType) {
            // don't ever hash the environment.
            visitor.visitHash(ShaHash(13));
            return;
        }

        // don't visit into constants
        if (isSimpleConstant(obj.pyobj())) {
            return;
        }

        Type* argType = PyInstance::extractTypeFrom(obj.pyobj()->ob_type);
        if (argType) {
            visitor.visitHash(ShaHash(2));
            visitor.visitTopo(argType);
            return;
        }

        // don't walk into canonical modules
        if (PyModule_Check(obj.pyobj())) {
            static PyObject* sysModule = ::sysModule();
            static PyObject* sysModuleModules = PyObject_GetAttrString(sysModule, "modules");

            PyObjectStealer name(PyObject_GetAttrString(obj.pyobj(), "__name__"));
            if (name) {
                if (PyUnicode_Check(name)) {
                    PyObjectStealer moduleObject(PyObject_GetItem(sysModuleModules, name));
                    if (moduleObject) {
                        if (moduleObject == obj.pyobj()) {
                            // this module is a canonical module. Lets not walk it as it's a standard
                            // system module
                            std::string moduleName = PyUnicode_AsUTF8(name);

                            //exclude modules that shouldn't change underneath us.
                            if (isCanonicalName(moduleName)) {
                                visitor.visitHash(ShaHash(12));
                                visitor.visitName(moduleName);
                                return;
                            }
                        }
                    } else {
                        PyErr_Clear();
                    }
                }
            } else {
                PyErr_Clear();
            }
        }

        // this might be a named object. Let's see if its name actually resolves it correctly,
        // in which case we can hash its name (and its contents if the compiler could see
        // through it)
        if (isPyObjectGloballyIdentifiableAndStable(obj.pyobj())) {
            std::string moduleName = std::string(PyUnicode_AsUTF8(PyObject_GetAttrString(obj.pyobj(), "__module__")));
            std::string clsName = std::string(PyUnicode_AsUTF8(PyObject_GetAttrString(obj.pyobj(), "__name__")));

            visitor.visitHash(ShaHash(2));
            visitor.visitName(moduleName + "|" + clsName);
            return;
        }

        if (PyType_Check(obj.pyobj())) {
            argType = PyInstance::extractTypeFrom((PyTypeObject*)obj.pyobj());
            if (argType) {
                visitor.visitHash(ShaHash(3));
                visitor.visitTopo(argType);
                return;
            }
        }

        if (PyCode_Check(obj.pyobj())) {
            PyCodeObject* co = (PyCodeObject*)obj.pyobj();

            visitor.visitHash(ShaHash(4));
            visitor.visitHash(ShaHash(co->co_argcount));
            visitor.visitHash(co->co_kwonlyargcount);
            visitor.visitHash(co->co_nlocals);
            visitor.visitHash(co->co_stacksize);
            // don't serialize the 'co_flags' field because it's not actually stable
            // and it doesn't contain any semantic information not available elsewhere.
            // visitor.visitHash(co->co_flags);
            visitor.visitHash(co->co_firstlineno);
            visitor.visitHash(ShaHash::SHA1(PyBytes_AsString(co->co_code), PyBytes_GET_SIZE(co->co_code)));
            visitTuple(co->co_consts);
            visitTuple(co->co_names);
            visitTuple(co->co_varnames);
            visitTuple(co->co_freevars);
            visitTuple(co->co_cellvars);
            // we ignore this, because otherwise, we'd have the hash change
            // whenever we instantiate code in a new location
            // visit(co->co_filename)
            visitor.visitTopo(co->co_name);

    #       if PY_MINOR_VERSION >= 10
                visitor.visitTopo(co->co_linetable);
    #       else
                visitor.visitTopo(co->co_lnotab);
    #       endif
            return;
        }

        if (PyFunction_Check(obj.pyobj())) {
            visitor.visitHash(ShaHash(5));

            PyFunctionObject* f = (PyFunctionObject*)obj.pyobj();

            if (f->func_closure) {
                visitor.visitHash(ShaHash(PyTuple_Size(f->func_closure)));

                for (long k = 0; k < PyTuple_Size(f->func_closure); k++) {
                    PyObject* o = PyTuple_GetItem(f->func_closure, k);
                    if (o && PyCell_Check(o)) {
                        visitor.visitTopo(o);
                    }
                }
            } else {
                visitor.visitHash(ShaHash(0));
            }

            visitor.visitTopo(f->func_name);
            visitor.visitTopo(PyFunction_GetCode((PyObject*)f));
            visitDictOrTuple(PyFunction_GetAnnotations((PyObject*)f));
            visitTuple(PyFunction_GetDefaults((PyObject*)f));
            visitDictOrTuple(PyFunction_GetKwDefaults((PyObject*)f));

            visitor.visitHash(ShaHash(1));

            if (f->func_globals && PyDict_Check(f->func_globals)) {

                std::vector<std::vector<PyObject*> > dotAccesses;

                Function::Overload::visitCompilerVisibleGlobals(
                    [&](std::string name, PyObject* val) {
                        if (!isSpecialIgnorableName(name)) {
                            visitor.visitNamedTopo(name, val);
                        }
                    },
                    (PyCodeObject*)f->func_code,
                    f->func_globals
                );
            }

            visitor.visitHash(ShaHash(0));
            return;
        }

        if (PyType_Check(obj.pyobj())) {
            visitor.visitHash(ShaHash(6));

            PyTypeObject* tp = (PyTypeObject*)obj.pyobj();

            visitor.visitHash(ShaHash(0));
            if (tp->tp_dict) {
                visitDict(tp->tp_dict, true);
            }
            visitor.visitHash(ShaHash(0));

            if (tp->tp_bases) {
                iterate(
                    tp->tp_bases,
                    [&](PyObject* t) { visitor.visitTopo(t); }
                );
            }

            visitor.visitHash(ShaHash(0));

            return;
        }

        if (obj.pyobj()->ob_type == &PyStaticMethod_Type || obj.pyobj()->ob_type == &PyClassMethod_Type) {
            if (obj.pyobj()->ob_type == &PyStaticMethod_Type) {
                visitor.visitHash(ShaHash(7));
            } else {
                visitor.visitHash(ShaHash(8));
            }

            PyObjectStealer funcObj(PyObject_GetAttrString(obj.pyobj(), "__func__"));

            if (!funcObj) {
                visitor.visitErr("not a func obj");
            } else {
                visitor.visitTopo((PyObject*)funcObj);
            }

            return;
        }

        if (PyTuple_Check(obj.pyobj())) {
            visitor.visitHash(ShaHash(9));
            visitor.visitHash(ShaHash(PyTuple_Size(obj.pyobj())));

            for (long k = 0; k < PyTuple_Size(obj.pyobj()); k++) {
                visitor.visitTopo(PyTuple_GetItem(obj.pyobj(), k));
            }

            return;
        }

        static PyObject* weakrefModule = ::weakrefModule();
        static PyObject* weakSetType = PyObject_GetAttrString(weakrefModule, "WeakSet");
        static PyObject* weakKeyDictType = PyObject_GetAttrString(weakrefModule, "WeakKeyDictionary");
        static PyObject* weakValueDictType = PyObject_GetAttrString(weakrefModule, "WeakValueDictionary");


        if (
            // dict, set and list are all mutable - we can't rely on their contents,
            // and the compiler shouldn't look inside of them anyways.
            PyDict_Check(obj.pyobj())
            || PySet_Check(obj.pyobj())
            || PyList_Check(obj.pyobj())
            // similarly, we shouldn't depend on the internals of a weakset/dict
            || obj.pyobj()->ob_type == (PyTypeObject*)weakSetType
            || obj.pyobj()->ob_type == (PyTypeObject*)weakKeyDictType
            || obj.pyobj()->ob_type == (PyTypeObject*)weakValueDictType
        ) {
            visitor.visitHash(ShaHash(10));
            visitor.visitTopo((PyObject*)obj.pyobj()->ob_type);
            return;
        }

        if (PyCell_Check(obj.pyobj())) {
            visitor.visitHash(ShaHash(11));

            if (PyCell_Get(obj.pyobj())) {
                visitor.visitHash(ShaHash(1));
                visitor.visitTopo(PyCell_Get(obj.pyobj()));
            } else {
                visitor.visitHash(ShaHash(0));
            }
            return;
        }

        if (obj.pyobj()->ob_type == &PyClassMethodDescr_Type
            || obj.pyobj()->ob_type == &PyMethodDescr_Type) {
            // the compiler looks at the type and the name of a given method descriptor
            visitor.visitTopo(PyDescr_TYPE(obj.pyobj()));
            visitor.visitTopo(PyDescr_NAME(obj.pyobj()));
            return;
        }

        // we don't visit the internals of arbitrary objects - by default, the compiler
        // won't do this because they are mutable.

        // we do visit the type, since the compiler may infer something about the type
        // of the instance and we assume that type objects are stable.
        visitor.visitTopo((PyObject*)obj.pyobj()->ob_type);
    }

    std::unordered_map<TypeOrPyobj, std::vector<VisitRecord> > mPastVisits;
};
