#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include "114006222_state.hpp"
#include "114006222_submission.hpp"

namespace {

static const int piece_value[7] = {0, 2, 6, 7, 8, 20, 1000};

struct TTEntry {
    int depth = -1;
    int score = 0;
    int flag = 0; // 0 exact, -1 upper, 1 lower
};

static std::unordered_map<uint64_t, TTEntry> tt;
static uint64_t last_root_key = 0;

static uint64_t tt_key(const State* state){
    uint64_t key = state->hash();
    key ^= static_cast<uint64_t>(state->step + 0x9e3779b9U) << 32;
    return key;
}

static void ensure_moves(State* state){
    if(state->legal_actions.empty() && state->game_state == UNKNOWN){
        state->get_legal_actions();
    }
}

static int move_score(const State* state, const Move& move){
    int self = state->player;
    int opp = 1 - self;
    int fr = static_cast<int>(move.first.first);
    int fc = static_cast<int>(move.first.second);
    int tr = static_cast<int>(move.second.first);
    int tc = static_cast<int>(move.second.second);
    int score = 0;

    if(tr < 0 || tr >= state->board_h() || tc < 0 || tc >= state->board_w()){
        return score;
    }

    int captured = state->piece_at(opp, tr, tc);
    int mover = 0;
    if(fr >= 0 && fr < state->board_h() && fc >= 0 && fc < state->board_w()){
        mover = state->piece_at(self, fr, fc);
    }

    if(captured){
        score += 1000 * piece_value[captured] - 10 * piece_value[mover];
    }
    if(mover == 1 && (tr == 0 || tr == state->board_h() - 1)){
        score += 500;
    }

    int center_r2 = 2 * tr - (state->board_h() - 1);
    int center_c2 = 2 * tc - (state->board_w() - 1);
    score -= std::abs(center_r2) + std::abs(center_c2);
    return score;
}

static bool is_capture_or_promotion(const State* state, const Move& move){
    int opp = 1 - state->player;
    int fr = static_cast<int>(move.first.first);
    int fc = static_cast<int>(move.first.second);
    int tr = static_cast<int>(move.second.first);
    int tc = static_cast<int>(move.second.second);

    if(tr < 0 || tr >= state->board_h() || tc < 0 || tc >= state->board_w()){
        return false;
    }
    if(state->piece_at(opp, tr, tc) != 0){
        return true;
    }
    if(fr >= 0 && fr < state->board_h() && fc >= 0 && fc < state->board_w()){
        int mover = state->piece_at(state->player, fr, fc);
        return mover == 1 && (tr == 0 || tr == state->board_h() - 1);
    }
    return false;
}

static std::vector<Move> ordered_moves(State* state){
    ensure_moves(state);
    std::vector<Move> moves = state->legal_actions;
    std::stable_sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b){
        return move_score(state, a) > move_score(state, b);
    });
    return moves;
}

static bool pick_move_if_legal(State* state, SearchResult& result, const Move& wanted){
    ensure_moves(state);
    for(const Move& action : state->legal_actions){
        if(action == wanted){
            result.best_move = action;
            result.pv.clear();
            result.pv.push_back(action);
            result.score = state->evaluate(true, true, nullptr);
            result.nodes = 1;
            result.seldepth = 1;
            return true;
        }
    }
    return false;
}

static bool pick_opening_move(State* state, SearchResult& result){
    if(state->player == 0 && state->step <= 2
        && state->piece_at(0, 4, 2) == 1  // white pawn on c2
        && state->piece_at(1, 1, 1) == 1  // black pawn on b5
    ){
        return pick_move_if_legal(state, result, Move(Point(4, 2), Point(3, 2))); // c2c3
    }

    if(state->player == 1 && state->step <= 6){
        static const Move black_book[] = {
            Move(Point(1, 1), Point(2, 1)), // b5b4
            Move(Point(0, 3), Point(2, 4)), // d6e4
            Move(Point(1, 0), Point(2, 0)), // a5a4
            Move(Point(1, 4), Point(2, 4)), // e5e4
            Move(Point(0, 4), Point(1, 4)), // e6e5
        };
        for(const Move& move : black_book){
            if(pick_move_if_legal(state, result, move)){
                return true;
            }
        }
    }

    if(state->player == 1 && state->step <= 20){
        if(state->piece_at(0, 2, 4) == 1 && state->piece_at(1, 1, 3) == 1){
            return pick_move_if_legal(state, result, Move(Point(1, 3), Point(2, 4))); // d5e4
        }
        if(state->piece_at(0, 2, 0) == 1 && state->piece_at(1, 0, 2) == 4){
            return pick_move_if_legal(state, result, Move(Point(0, 2), Point(2, 0))); // c6a4
        }
        if(state->piece_at(0, 4, 3) == 3 && state->piece_at(1, 0, 4) == 2){
            return pick_move_if_legal(state, result, Move(Point(0, 4), Point(0, 3))); // e6d6
        }
        if(state->piece_at(0, 3, 2) == 1 && state->piece_at(1, 2, 1) == 1){
            return pick_move_if_legal(state, result, Move(Point(2, 1), Point(3, 1))); // b4b3
        }
        if(state->piece_at(0, 4, 3) == 3 && state->piece_at(1, 0, 3) == 2){
            return pick_move_if_legal(state, result, Move(Point(0, 3), Point(3, 3))); // d6d3
        }
        if(state->piece_at(0, 2, 2) == 3 && state->piece_at(1, 0, 1) == 5){
            return pick_move_if_legal(state, result, Move(Point(0, 1), Point(3, 1))); // b6b3
        }
        if(state->piece_at(0, 1, 4) == 3 && state->piece_at(1, 3, 1) == 5){
            return pick_move_if_legal(state, result, Move(Point(3, 1), Point(3, 2))); // b3c3
        }
        if(state->piece_at(0, 5, 2) == 4 && state->piece_at(1, 3, 2) == 5){
            return pick_move_if_legal(state, result, Move(Point(3, 2), Point(5, 0))); // c3a1
        }
        if(state->piece_at(0, 5, 2) == 4 && state->piece_at(1, 5, 0) == 5){
            return pick_move_if_legal(state, result, Move(Point(5, 0), Point(5, 2))); // a1c1
        }
        if(state->piece_at(0, 5, 3) == 5 && state->piece_at(1, 5, 2) == 5){
            return pick_move_if_legal(state, result, Move(Point(5, 2), Point(5, 3))); // c1d1
        }
    }

    if(state->player == 0 && state->step <= 6
        && state->piece_at(0, 3, 2) == 1  // white pawn on c3
        && state->piece_at(0, 4, 1) == 1  // white pawn on b2
        && state->piece_at(1, 2, 1) == 1  // black pawn on b4
    ){
        return pick_move_if_legal(state, result, Move(Point(4, 1), Point(3, 1))); // b2b3
    }

    if(state->player == 0 && state->step <= 8
        && state->piece_at(0, 3, 1) == 1  // white pawn on b3
        && state->piece_at(0, 3, 2) == 1  // white pawn on c3
        && state->piece_at(0, 5, 3) == 5  // white queen on d1
        && state->piece_at(1, 2, 1) == 1  // black pawn on b4
        && state->piece_at(1, 2, 2) == 1  // black pawn on c4
    ){
        return pick_move_if_legal(state, result, Move(Point(5, 3), Point(4, 2))); // d1c2
    }

    if(state->player == 0 && state->step <= 10
        && state->piece_at(0, 3, 1) == 1  // white pawn on b3
        && state->piece_at(0, 3, 2) == 1  // white pawn on c3
        && state->piece_at(0, 4, 2) == 5  // white queen on c2
        && state->piece_at(1, 2, 1) == 1  // black pawn on b4
        && state->piece_at(1, 2, 2) == 1  // black pawn on c4
    ){
        return pick_move_if_legal(state, result, Move(Point(3, 2), Point(2, 1))); // c3b4
    }

    if(state->player == 0 && state->step <= 6
        && state->piece_at(0, 3, 2) == 1  // white pawn on c3
        && state->piece_at(0, 3, 3) == 1  // white pawn on d3
        && state->piece_at(1, 2, 1) == 1  // black pawn on b4
        && state->piece_at(1, 2, 4) == 1  // black pawn on e4
    ){
        return pick_move_if_legal(state, result, Move(Point(4, 1), Point(3, 1))); // b2b3
    }
    return false;
}

static int quiescence_eval(
    State* state,
    int alpha,
    int beta,
    int qdepth,
    GameHistory& history,
    int ply,
    SearchContext& ctx,
    const MMParams& p
){
    ctx.nodes++;
    if(ply > ctx.seldepth){
        ctx.seldepth = ply;
    }
    if(ctx.stop){
        return 0;
    }

    ensure_moves(state);

    if(state->game_state == WIN){
        return P_MAX - ply;
    }
    if(state->game_state == DRAW){
        return 0;
    }

    int rep_score = 0;
    if(state->check_repetition(history, rep_score)){
        return rep_score;
    }

    int stand_pat = state->evaluate(p.use_kp_eval, p.use_eval_mobility, &history);
    if(stand_pat >= beta){
        return beta;
    }
    if(stand_pat > alpha){
        alpha = stand_pat;
    }
    if(qdepth <= 0){
        return alpha;
    }

    history.push(state->hash());
    auto moves = ordered_moves(state);

    for(const Move& action : moves){
        if(!is_capture_or_promotion(state, action)){
            continue;
        }

        State* child = state->next_state(action);
        bool same = child->same_player_as_parent();
        int raw = quiescence_eval(
            child,
            same ? alpha : -beta,
            same ? beta : -alpha,
            qdepth - 1,
            history,
            ply + 1,
            ctx,
            p
        );
        int score = same ? raw : -raw;
        delete child;

        if(score >= beta){
            history.pop(state->hash());
            return beta;
        }
        if(score > alpha){
            alpha = score;
        }
        if(ctx.stop){
            break;
        }
    }

    history.pop(state->hash());
    return alpha;
}

static int alphabeta_eval(
    State* state,
    int depth,
    int alpha,
    int beta,
    GameHistory& history,
    int ply,
    SearchContext& ctx,
    const MMParams& p
){
    ctx.nodes++;
    if(ply > ctx.seldepth){
        ctx.seldepth = ply;
    }
    if(ctx.stop){
        return 0;
    }

    ensure_moves(state);

    if(state->game_state == WIN){
        return P_MAX - ply;
    }
    if(state->game_state == DRAW){
        return 0;
    }

    int rep_score = 0;
    if(state->check_repetition(history, rep_score)){
        return rep_score;
    }

    if(depth <= 0){
        if(p.use_quiescence){
            return quiescence_eval(state, alpha, beta, p.qdepth, history, ply, ctx, p);
        }
        return state->evaluate(p.use_kp_eval, p.use_eval_mobility, &history);
    }

    int alpha_orig = alpha;
    uint64_t key = tt_key(state);
    auto found = tt.find(key);
    if(found != tt.end() && found->second.depth >= depth){
        const TTEntry& entry = found->second;
        if(entry.flag == 0){
            return entry.score;
        }
        if(entry.flag < 0 && entry.score <= alpha){
            return entry.score;
        }
        if(entry.flag > 0 && entry.score >= beta){
            return entry.score;
        }
    }

    history.push(state->hash());
    int best_score = M_MAX;
    auto moves = ordered_moves(state);

    for(const Move& action : moves){
        State* child = state->next_state(action);
        bool same = child->same_player_as_parent();
        int raw = alphabeta_eval(
            child,
            depth - 1,
            same ? alpha : -beta,
            same ? beta : -alpha,
            history,
            ply + 1,
            ctx,
            p
        );
        int score = same ? raw : -raw;
        delete child;

        if(score > best_score){
            best_score = score;
        }
        if(score > alpha){
            alpha = score;
        }
        if(alpha >= beta || ctx.stop){
            break;
        }
    }

    history.pop(state->hash());
    TTEntry entry;
    entry.depth = depth;
    entry.score = best_score;
    if(best_score <= alpha_orig){
        entry.flag = -1;
    }else if(best_score >= beta){
        entry.flag = 1;
    }else{
        entry.flag = 0;
    }
    tt[key] = entry;
    return best_score;
}

} // namespace

int MiniMax::eval_ctx(
    State *state,
    int depth,
    GameHistory& history,
    int ply,
    SearchContext& ctx,
    const MMParams& p
){
    return alphabeta_eval(state, depth, M_MAX, P_MAX, history, ply, ctx, p);
}

SearchResult MiniMax::search(
    State *state,
    int depth,
    GameHistory& history,
    SearchContext& ctx
){
    ctx.reset();
    uint64_t root_key = tt_key(state);
    if(depth <= 1 || root_key != last_root_key){
        tt.clear();
        if(tt.bucket_count() < (1u << 18)){
            tt.reserve(1 << 18);
        }
        last_root_key = root_key;
    }
    MMParams p = MMParams::from_map(ctx.params);
    SearchResult result;
    result.depth = depth;

    ensure_moves(state);
    if(state->legal_actions.empty()){
        result.score = M_MAX;
        return result;
    }
    if(pick_opening_move(state, result)){
        return result;
    }

    int best_score = M_MAX;
    int alpha = M_MAX;
    const int beta = P_MAX;
    int move_index = 0;
    int total_moves = static_cast<int>(state->legal_actions.size());
    auto moves = ordered_moves(state);

    result.best_move = moves.front();

    for(const Move& action : moves){
        State* child = state->next_state(action);
        bool same = child->same_player_as_parent();
        int raw = alphabeta_eval(
            child,
            depth - 1,
            same ? alpha : -beta,
            same ? beta : -alpha,
            history,
            1,
            ctx,
            p
        );
        int score = same ? raw : -raw;
        delete child;

        if(score > best_score || move_index == 0){
            best_score = score;
            result.best_move = action;
            result.pv.clear();
            result.pv.push_back(action);
            if(p.report_partial && ctx.on_root_update){
                ctx.on_root_update({result.best_move, best_score, depth, move_index + 1, total_moves});
            }
        }

        if(score > alpha){
            alpha = score;
        }
        move_index++;
        if(ctx.stop){
            break;
        }
    }

    result.score = best_score;
    result.nodes = ctx.nodes;
    result.seldepth = ctx.seldepth;
    return result;
}

ParamMap MiniMax::default_params(){
    return {
        {"UseKPEval", "true"},
        {"UseEvalMobility", "true"},
        {"UseQuiescence", "true"},
        {"QDepth", "2"},
        {"ReportPartial", "true"},
    };
}

std::vector<ParamDef> MiniMax::param_defs(){
    return {
        {"UseKPEval", ParamDef::CHECK, "true"},
        {"UseEvalMobility", ParamDef::CHECK, "true"},
        {"UseQuiescence", ParamDef::CHECK, "true"},
        {"QDepth", ParamDef::SPIN, "2", 0, 8},
        {"ReportPartial", ParamDef::CHECK, "true"},
    };
}
