#pragma once
#include "search_types.hpp"
#include "game_history.hpp"
#include "114006222_submission.hpp"

class Quiescence {
public:
    static int eval_ctx(
        State *state,
        int depth,
        int alpha,
        int beta,
        GameHistory& history,
        int ply,
        SearchContext& ctx,
        const MMParams& p
    );

    // Added a separate execution context dedicated to parsing captures
    static int qsearch(
        State *state,
        int alpha,
        int beta,
        GameHistory& history,
        int ply,
        SearchContext& ctx,
        const MMParams& p
    );

    static SearchResult search(
        State *state,
        int depth,
        GameHistory& history,
        SearchContext& ctx
    );

    static ParamMap default_params();
    static std::vector<ParamDef> param_defs();
};