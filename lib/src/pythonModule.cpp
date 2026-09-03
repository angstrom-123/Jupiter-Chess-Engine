#include <csignal>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>

#include <Python.h>
#include <object.h>
#include <methodobject.h>
#include <pytypedefs.h>
#include <unicodeobject.h>

#include "util/instrumenter.h"
#include "libjupiter/board.h"
#include "movegen/move.h"

// Wrappers

void SegfaultHandler(int signal)
{
    JUPITER_TRACE();

    ERROR("\n=== FATAL (" << signal << ") ===");
    StackTracer::PrintTrace();
    std::terminate();
}

namespace py {
    struct Board {
        PyObject_HEAD
        libjupiter::Board* board;
    };

    static void BoardDealloc(Board *self)
    {
        JUPITER_TRACE();

        delete self->board;
        Py_TYPE(self)->tp_free(reinterpret_cast<PyObject *>(self));
    }

    static PyObject *BoardNew(PyTypeObject *type, PyObject *, PyObject *)
    {
        JUPITER_TRACE();

        Board *self = reinterpret_cast<Board *>(type->tp_alloc(type, 0));
        if (self)
            self->board = nullptr;
        return reinterpret_cast<PyObject *>(self);
    }

    static int BoardInit(Board *self, PyObject *args, PyObject *)
    {
        JUPITER_TRACE();

        char *fen = nullptr;
        if (!PyArg_ParseTuple(args, "|s", &fen))
            return -1;
        self->board = new libjupiter::Board(fen);
        return 0;
    }

    // Methods

    static PyObject *BoardRepr(Board *self)
    {
        JUPITER_TRACE();

        if (!self->board)
            return nullptr;
        std::string result;
        self->board->Show(result);
        return PyUnicode_FromString(result.c_str());
    }

    static PyObject *BoardGetMetrics(Board *self, PyObject *)
    {
        JUPITER_TRACE();

        if (!self->board)
            return nullptr;
        std::string result;
        self->board->GetMetrics(result);
        return PyUnicode_FromString(result.c_str());
    }

    static PyObject *BoardGetTelemetry(Board *self, PyObject *)
    {
        JUPITER_TRACE();

        if (!self->board)
            return nullptr;
        std::string result;
        self->board->GetTelemetry(result);
        return PyUnicode_FromString(result.c_str());
    }

    static PyObject *BoardGo(Board *self, PyObject *args)
    {
        JUPITER_TRACE();

        if (!self->board)
            return nullptr;
        uint64_t ms;
        if (!PyArg_ParseTuple(args, "K", &ms))
            return nullptr;
        Move move = self->board->Go(ms);
        if (move.IsValid()) {
            const LongAlgebraicMove lan = move.ToLAN();

            char safeChars[8] = { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };
            for (std::size_t i = 0; i < 5; i++)
                safeChars[i] = lan.chars[i];

            return PyUnicode_FromString(safeChars);
        }
        Py_RETURN_NONE;
    }

    static PyObject *BoardMakeMove(Board *self, PyObject *args)
    {
        JUPITER_TRACE();

        if (!self->board)
            return nullptr;
        char *lan = nullptr;
        if (!PyArg_ParseTuple(args, "s", &lan))
            return nullptr;

        LongAlgebraicMove move = LongAlgebraicMove::FromChars(lan);
        if (!move.IsValid())
            return nullptr;
        self->board->MakeMove(move);
        Py_RETURN_NONE;
    }

    static PyObject *BoardSetTimeControl(Board *self, PyObject *args)
    {
        JUPITER_TRACE();

        if (!self->board)
            return nullptr;
        uint64_t seconds;
        uint64_t increment;
        if (!PyArg_ParseTuple(args, "KK", &seconds, &increment))
            return nullptr;

        self->board->SetTimeControl(seconds, increment);
        Py_RETURN_NONE;
    }

    // Board Method Table

    static PyMethodDef boardMethods[] = {
        { "go", reinterpret_cast<PyCFunction>(py::BoardGo), METH_VARARGS, "Find the best move on the current board within the given time." },
        { "make_move", reinterpret_cast<PyCFunction>(py::BoardMakeMove), METH_VARARGS, "Apply a move in Long Algebraic Notation to update game state." },
        { "set_time_control", reinterpret_cast<PyCFunction>(py::BoardSetTimeControl), METH_VARARGS, "Set the time control for the engine to use." },
        { "get_telemetry", reinterpret_cast<PyCFunction>(py::BoardGetTelemetry), METH_NOARGS, "Get internal engine telemetry as stringified JSON." },
        { "get_metrics", reinterpret_cast<PyCFunction>(py::BoardGetMetrics), METH_NOARGS, "Get internal engine metrics as stringified JSON." },
        { nullptr, nullptr, 0, nullptr }
    };

    // Board Type Definition

    static PyTypeObject boardType = {
        .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
        .tp_name = "libjupiter.Board",
        .tp_basicsize = sizeof(libjupiter::Board),
        .tp_itemsize = 0,
        .tp_dealloc = reinterpret_cast<destructor>(BoardDealloc),
        .tp_repr = reinterpret_cast<reprfunc>(BoardRepr),
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_doc = "Chess board class",
        .tp_weaklistoffset = 0,
        .tp_methods = boardMethods,
        .tp_init = reinterpret_cast<initproc>(BoardInit),
        .tp_new = BoardNew,
    };
}

// Module Method Table

static PyMethodDef libjupiterMethods[] = {
    { nullptr, nullptr, 0, nullptr }
};

// Module Definition

static PyModuleDef libjupiterModule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "libjupiter",
    .m_doc = nullptr,
    .m_size = -1,
    .m_methods = libjupiterMethods,
};

// Module Init

PyMODINIT_FUNC PyInit_libjupiter(void)
{
    std::signal(SIGSEGV, SegfaultHandler);
    std::signal(SIGILL, SegfaultHandler);
    std::signal(SIGFPE, SegfaultHandler);
    PyObject * module = PyModule_Create(&libjupiterModule);
    if (!module)
        return nullptr;

    if (PyType_Ready(&py::boardType) < 0)
        return nullptr;

    Py_INCREF(&py::boardType);
    PyModule_AddObject(module, "Board", reinterpret_cast<PyObject *>(&py::boardType));

    return module;
}
