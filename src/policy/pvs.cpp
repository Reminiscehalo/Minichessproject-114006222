#include <algorithm>
#include "114006222_state.hpp"
#include "pvs.hpp"

int PVS::eval_ctx(State *state, int depth, int alpha, int beta, GameHistory& history, int ply, SearchContext& ctx, const MMParams& p) {
    ctx.nodes++;
    if(ply > ctx.seldepth) ctx.seldepth = ply;
    if(ctx.stop) return 0;

    if(state->legal_actions.empty() && state->game_state == UNKNOWN) state->get_legal_actions();
    std::sort(
        state->legal_actions.begin(),
        state->legal_actions.end(),
        [&](const Move& a, const Move& b)
        {
            int opp = 1 - state->player;

            int capturedA =
                state->board.board[opp]
                                [a.second.first]
                                [a.second.second];

            int capturedB =
                state->board.board[opp]
                                [b.second.first]
                                [b.second.second];

            return capturedA > capturedB;
        }
    );

    if(state->game_state == WIN) return -M_MAX + ply;
    if(state->game_state == DRAW) return 0;

    int rep_score;
    if(state->check_repetition(history, rep_score)) return rep_score;
    history.push(state->hash());

    if(depth <= 0) {
        int score = state->evaluate(p.use_kp_eval, p.use_eval_mobility, &history);
        history.pop(state->hash());
        return score;
    }

    int best_score = -M_MAX;
    bool is_first_move = true;

    for(auto& action : state->legal_actions) {
        State *next = state->next_state(action);
        bool same = next->same_player_as_parent();
        int score;

        if(is_first_move) {
            // 1. Search primary variation with full window
            int child_alpha = same ? alpha : -beta;
            int child_beta = same ? beta : -alpha;
            int raw_score = eval_ctx(next, depth - 1, child_alpha, child_beta, history, ply + 1, ctx, p);
            score = same ? raw_score : -raw_score;
            is_first_move = false;
        } else {
            // 2. Search subsequent moves with a null-window
            int child_alpha = same ? alpha : -alpha - 1;
            int child_beta = same ? alpha + 1 : -alpha;
            int raw_score = eval_ctx(next, depth - 1, child_alpha, child_beta, history, ply + 1, ctx, p);
            score = same ? raw_score : -raw_score;

            // 3. Re-search with full window if the null-window failed high
            if(score > alpha && score < beta) {
                int full_alpha = same ? alpha : -beta;
                int full_beta = same ? beta : -alpha;
                raw_score = eval_ctx(next, depth - 1, full_alpha, full_beta, history, ply + 1, ctx, p);
                score = same ? raw_score : -raw_score;
            }
        }
        delete next;

        if(score > best_score) best_score = score;
        alpha = std::max(alpha, best_score);
        if(alpha >= beta) break; 
    }

    history.pop(state->hash());
    return best_score;
}

SearchResult PVS::search(State *state, int depth, GameHistory& history, SearchContext& ctx) {
    ctx.reset();
    MMParams p = MMParams::from_map(ctx.params);
    SearchResult result;
    result.depth = depth;

    if(state->legal_actions.empty()) state->get_legal_actions();
    
    if(state->legal_actions.empty())
    {
        result.score = -M_MAX;
        return result;
    }

    result.best_move = state->legal_actions.front();

    std::sort(
        state->legal_actions.begin(),
        state->legal_actions.end(),
        [&](const Move& a, const Move& b)
        {
            int opp = 1 - state->player;

            int capturedA =
                state->board.board[opp]
                                [a.second.first]
                                [a.second.second];

            int capturedB =
                state->board.board[opp]
                                [b.second.first]
                                [b.second.second];

            return capturedA > capturedB;
        }
    );


    int best_score = -M_MAX;
    int alpha = -M_MAX;
    int beta = M_MAX;
    int move_index = 0;
    int total_moves = (int)state->legal_actions.size();
    bool is_first_move = true;

    for(auto& action : state->legal_actions) {
        State *child = state->next_state(action);
        bool same = child->same_player_as_parent();
        int score;

        if(is_first_move) {
            int child_alpha = same ? alpha : -beta;
            int child_beta = same ? beta : -alpha;
            int raw_score = eval_ctx(child, depth - 1, child_alpha, child_beta, history, 1, ctx, p);
            score = same ? raw_score : -raw_score;
            is_first_move = false;
        } else {
            int child_alpha = same ? alpha : -alpha - 1;
            int child_beta = same ? alpha + 1 : -alpha;
            int raw_score = eval_ctx(child, depth - 1, child_alpha, child_beta, history, 1, ctx, p);
            score = same ? raw_score : -raw_score;

            if(score > alpha && score < beta) {
                int full_alpha = same ? alpha : -beta;
                int full_beta = same ? beta : -alpha;
                raw_score = eval_ctx(child, depth - 1, full_alpha, full_beta, history, 1, ctx, p);
                score = same ? raw_score : -raw_score;
            }
        }
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
ParamMap PVS::default_params() { return MiniMax::default_params(); }
std::vector<ParamDef> PVS::param_defs() { return MiniMax::param_defs(); }