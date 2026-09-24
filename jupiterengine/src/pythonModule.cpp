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

#include "evaluation/evaluator.h"
#include "modsupport.h"
#include "util/instrumenter.h"
#include "libjupiter/board.h"
#include "movegen/move.h"
#include "util/stackTrace.h"

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
        if (self) self->board = nullptr;
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
        float seconds;
        float increment;
        if (!PyArg_ParseTuple(args, "ff", &seconds, &increment))
            return nullptr;

        self->board->SetTimeControl(seconds, increment);
        Py_RETURN_NONE;
    }

    static PyObject *BoardSetWeights(Board *self, PyObject *args, PyObject *keywords)
    {
        JUPITER_TRACE();

        if (!self->board)
            return nullptr;

        EvaluatorConstants w = self->board->GetWeights();

        static char *kwlist[] = {
            const_cast<char *>("material_weight"),
            const_cast<char *>("pst_weight"),
            const_cast<char *>("mopup_proximity_factor"),
            const_cast<char *>("mopup_edge_factor"),
            const_cast<char *>("king_mobility_factor"),
            const_cast<char *>("mobility_factor"),
            const_cast<char *>("king_pawn_tropism_regular_factor"),
            const_cast<char *>("king_pawn_tropism_weak_factor"),
            const_cast<char *>("king_pawn_tropism_passed_factor"),
            const_cast<char *>("missing_shield_pawn_factor"),
            const_cast<char *>("storming_pawn_factor"),
            const_cast<char *>("slider_open_file_factor"),
            const_cast<char *>("weak_pawn_factor"),
            const_cast<char *>("connected_pawn_factor"),
            nullptr
        };
        bool parsed = PyArg_ParseTupleAndKeywords(args, keywords, "|ffiiiiiiiiiiii", kwlist, 
            &w.materialWeight, 
            &w.pstWeight, 
            &w.mopupProximityFactor, 
            &w.mopupEdgeFactor,
            &w.kingMobilityFactor,
            &w.mobilityFactor,
            &w.kingPawnTropismFactors[PawnKind::REGULAR],
            &w.kingPawnTropismFactors[PawnKind::WEAK],
            &w.kingPawnTropismFactors[PawnKind::PASSED],
            &w.missingShieldPawnFactor,
            &w.stormingPawnFactor,
            &w.sliderOpenFileFactor,
            &w.weakPawnFactor,
            &w.connectedPawnFactor);
        if (!parsed)
            return nullptr;

        self->board->SetWeights(w);
        Py_RETURN_NONE;
    }

    static PyObject *BoardGetWeights(Board *self, PyObject *)
    {
        JUPITER_TRACE();

        if (!self->board)
            return nullptr;

        EvaluatorConstants w = self->board->GetWeights();

        PyObject *dict = PyDict_New();
        if (!dict)
            return nullptr;

        PyObject *materialWeight = PyFloat_FromDouble(w.materialWeight);
        PyObject *pstWeight = PyFloat_FromDouble(w.pstWeight);
        PyObject *mopupProximityFactor = PyLong_FromLong(w.mopupProximityFactor);
        PyObject *mopupEdgeFactor = PyLong_FromLong(w.mopupEdgeFactor);
        PyObject *kingMobilityFactor = PyLong_FromLong(w.kingMobilityFactor);
        PyObject *mobilityFactor = PyLong_FromLong(w.mobilityFactor);
        PyObject *kingPawnTropismRegularFactor = PyLong_FromLong(w.kingPawnTropismFactors[PawnKind::REGULAR]);
        PyObject *kingPawnTropismWeakFactor  = PyLong_FromLong(w.kingPawnTropismFactors[PawnKind::WEAK]);
        PyObject *kingPawnTropismPassedFactor   = PyLong_FromLong(w.kingPawnTropismFactors[PawnKind::PASSED]);
        PyObject *missingShieldPawnFactor = PyLong_FromLong(w.missingShieldPawnFactor);
        PyObject *stormingPawnFactor = PyLong_FromLong(w.stormingPawnFactor);
        PyObject *sliderOpenFileFactor = PyLong_FromLong(w.sliderOpenFileFactor);
        PyObject *weakPawnFactor = PyLong_FromLong(w.weakPawnFactor);
        PyObject *connectedPawnFactor = PyLong_FromLong(w.connectedPawnFactor);

        PyDict_SetItemString(dict, "material_weight", materialWeight);
        PyDict_SetItemString(dict, "pst_weight", pstWeight);
        PyDict_SetItemString(dict, "mopup_proximity_factor", mopupProximityFactor);
        PyDict_SetItemString(dict, "mopup_edge_factor", mopupEdgeFactor);
        PyDict_SetItemString(dict, "king_mobility_factor", kingMobilityFactor);
        PyDict_SetItemString(dict, "mobility_factor", mobilityFactor);
        PyDict_SetItemString(dict, "king_pawn_tropism_regular_factor", kingPawnTropismRegularFactor);
        PyDict_SetItemString(dict, "king_pawn_tropism_weak_factor", kingPawnTropismWeakFactor);
        PyDict_SetItemString(dict, "king_pawn_tropism_passed_factor", kingPawnTropismPassedFactor);
        PyDict_SetItemString(dict, "missing_shield_pawn_factor", missingShieldPawnFactor);
        PyDict_SetItemString(dict, "storming_pawn_factor", stormingPawnFactor);
        PyDict_SetItemString(dict, "slider_open_file_factor", sliderOpenFileFactor);
        PyDict_SetItemString(dict, "weak_pawn_factor", weakPawnFactor);
        PyDict_SetItemString(dict, "connected_pawn_factor", connectedPawnFactor);

        Py_DECREF(materialWeight);
        Py_DECREF(pstWeight);
        Py_DECREF(mopupProximityFactor);
        Py_DECREF(mopupEdgeFactor);
        Py_DECREF(kingMobilityFactor);
        Py_DECREF(mobilityFactor);
        Py_DECREF(kingPawnTropismRegularFactor);
        Py_DECREF(kingPawnTropismWeakFactor);
        Py_DECREF(kingPawnTropismPassedFactor);
        Py_DECREF(missingShieldPawnFactor);
        Py_DECREF(stormingPawnFactor);
        Py_DECREF(sliderOpenFileFactor);
        Py_DECREF(weakPawnFactor);
        Py_DECREF(connectedPawnFactor);

        return dict;
    }

    // Board Method Table

    #define PYCFN(fn) reinterpret_cast<PyCFunction>(fn)
    static PyMethodDef boardMethods[] = {
        { "go",               PYCFN(BoardGo),             METH_VARARGS,                 "Find the best move on the current board within the given time." },
        { "make_move",        PYCFN(BoardMakeMove),       METH_VARARGS,                 "Apply a move in Long Algebraic Notation to update game state." },
        { "set_time_control", PYCFN(BoardSetTimeControl), METH_VARARGS,                 "Set the time control for the engine to use." },
        { "get_telemetry",    PYCFN(BoardGetTelemetry),   METH_NOARGS,                  "Get internal engine telemetry as stringified JSON." },
        { "get_metrics",      PYCFN(BoardGetMetrics),     METH_NOARGS,                  "Get internal engine metrics as stringified JSON." },
        { "set_weights",      PYCFN(BoardSetWeights),     METH_VARARGS | METH_KEYWORDS, "Adjust evaluation weights and factors from provided defaults." },
        { "get_weights",      PYCFN(BoardGetWeights),     METH_NOARGS,                  "Get current evaluation weights as a dict." },
        { nullptr, nullptr, 0, nullptr }
    };

    // Board Type Definition
    static PyTypeObject BoardType = {
        .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
        .tp_name = "libjupiter.Board",
        .tp_basicsize = sizeof(libjupiter::Board),
        .tp_itemsize = 0,
        .tp_dealloc = reinterpret_cast<destructor>(BoardDealloc),
        .tp_vectorcall_offset = 0,
        .tp_getattr = nullptr,
        .tp_setattr = nullptr,
        .tp_as_async = nullptr,
        .tp_repr = reinterpret_cast<reprfunc>(BoardRepr),
        .tp_as_number = nullptr,
        .tp_as_sequence = nullptr,
        .tp_as_mapping = nullptr,
        .tp_hash = nullptr,
        .tp_call = nullptr,
        .tp_str = nullptr,
        .tp_getattro = nullptr,
        .tp_setattro = nullptr,
        .tp_as_buffer = nullptr,
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_doc = "Chess board class",
        .tp_traverse = nullptr,
        .tp_clear = nullptr,
        .tp_richcompare = nullptr,
        .tp_weaklistoffset = 0,
        .tp_iter = nullptr,
        .tp_iternext = nullptr,
        .tp_methods = boardMethods,
        .tp_members = nullptr,
        .tp_getset = nullptr,
        .tp_base = nullptr,
        .tp_dict = nullptr,
        .tp_descr_get = nullptr,
        .tp_descr_set = nullptr,
        .tp_dictoffset = 0,
        .tp_init = reinterpret_cast<initproc>(BoardInit),
        .tp_alloc = nullptr,
        .tp_new = BoardNew,
        .tp_free = nullptr,
        .tp_is_gc = nullptr,
        .tp_bases = nullptr,
        .tp_mro = nullptr,
        .tp_cache = nullptr,
        .tp_subclasses = nullptr,
        .tp_weaklist = nullptr,
        .tp_del = nullptr,
        .tp_version_tag = 0,
        .tp_finalize = nullptr,
        .tp_vectorcall = nullptr,
        .tp_watched = 0
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
    .m_slots = nullptr,
    .m_traverse = nullptr,
    .m_clear = nullptr,
    .m_free = nullptr
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

    if (PyType_Ready(&py::BoardType) < 0)
        return nullptr;

    Py_INCREF(&py::BoardType);
    PyModule_AddObject(module, "Board", reinterpret_cast<PyObject *>(&py::BoardType));

    return module;
}
