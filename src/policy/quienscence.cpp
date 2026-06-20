#include <algorithm>
#include "114006222_state.hpp"
#include "quienscence.hpp"

int Quiescence::qsearch(State *state, int alpha, int beta, GameHistory& history, int ply, SearchContext& ctx, const MMParams& p) {
    ctx.nodes++;
    if(ctx.stop) return 0;

    // Stand-pat: Score the board as a baseline baseline evaluation
    int stand_pat = state->evaluate(p.use_kp_eval, p.use_eval_mobility, &history);
    if(stand_pat >= beta) return stand_pat;
    if(stand_pat > alpha) alpha = stand_pat;

    if(state->legal_actions.empty() && state->game_state == UNKNOWN) state->get_legal_actions();

    int oppn = 1 - state->player; // Accessing opponent matrix indexes directly from state layout

    for(auto& action : state->legal_actions) {
        // Evaluate destination coordinates: check if an opponent piece is captured
        bool is_capture = state->board.board[oppn][action.second.first][action.second.second] > 0;
        if(!is_capture) continue; // Skip non-tactical positional moves entirely

        State *next = state->next_state(action);
        bool same = next->same_player_as_parent();

        int child_alpha = same ? alpha : -beta;
        int child_beta = same ? beta : -alpha;

        int raw_score = qsearch(next, child_alpha, child_beta, history, ply + 1, ctx, p);
        int score = same ? raw_score : -raw_score;
        delete next;

        if(score >= beta) return score;
        if(score > alpha) alpha = score;
    }
    return alpha;
}

int Quiescence::eval_ctx(State *state, int depth, int alpha, int beta, GameHistory& history, int ply, SearchContext& ctx, const MMParams& p) {
    ctx.nodes++;
    if(ply > ctx.seldepth) ctx.seldepth = ply;
    if(ctx.stop) return 0;

    if(state->legal_actions.empty() && state->game_state == UNKNOWN) state->get_legal_actions();
    if(state->game_state == WIN) return -M_MAX + ply;
    if(state->game_state == DRAW) return 0;

    int rep_score;
    if(state->check_repetition(history, rep_score)) return rep_score;
    history.push(state->hash());

    // Divergence point: swap standard search for selective capture evaluation at depth limit
    if(depth <= 0) {
        int score = qsearch(state, alpha, beta, history, ply, ctx, p);
        history.pop(state->hash());
        return score;
    }

    int best_score = -M_MAX;
    for(auto& action : state->legal_actions) {
        State *next = state->next_state(action);
        bool same = next->same_player_as_parent();

        int child_alpha = same ? alpha : -beta;
        int child_beta = same ? beta : -alpha;

        int raw_score = eval_ctx(next, depth - 1, child_alpha, child_beta, history, ply + 1, ctx, p);
        int score = same ? raw_score : -raw_score;
        delete next;

        if(score > best_score) best_score = score;
        alpha = std::max(alpha, best_score);
        if(alpha >= beta) break;
    }

    history.pop(state->hash());
    return best_score;
}

SearchResult Quiescence::search(State *state, int depth, GameHistory& history, SearchContext& ctx) {
    
    ctx.reset();
    MMParams p = MMParams::from_map(ctx.params);
    SearchResult result;
    result.depth = depth;

    if(state->legal_actions.empty()) state->get_legal_actions();

    int best_score = -M_MAX;
    int alpha = -M_MAX;
    int beta = M_MAX;
    int move_index = 0;
    int total_moves = (int)state->legal_actions.size();

    for(auto& action : state->legal_actions) {
        State *child = state->next_state(action);
        bool same = child->same_player_as_parent();

        int child_alpha = same ? alpha : -beta;
        int child_beta = same ? beta : -alpha;

        int raw_score = eval_ctx(child, depth - 1, child_alpha, child_beta, history, 1, ctx, p);
        int score = same ? raw_score : -raw_score;
        delete child;

        if(score > best_score) {
            best_score = score;
            result.best_move = action;
            if(p.report_partial && ctx.on_root_update) {
                ctx.on_root_update({result.best_move, best_score, depth, move_index + 1, total_moves});
            }
        }
        alpha = std::max(alpha, best_score);
        move_index++;
    }
    result.score = best_score;
    return result;
}

// Missing parameter utilities mapping to MiniMax base
ParamMap Quiescence::default_params() { return MiniMax::default_params(); }
std::vector<ParamDef> Quiescence::param_defs() { return MiniMax::param_defs(); }