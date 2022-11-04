#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
using namespace eosio;
using namespace std;

CONTRACT tictactoe : public contract
{

public:
    using contract::contract;

    tictactoe(name receiver, name code, datastream<const char *> ds) : contract(receiver, code, ds), betting_token_symbol("ASA", 4)
    {
    }

    TABLE games
    {
        // name host;
        name opponent;
        int64_t bet;
        bool status;
        auto primary_key() const { return opponent.value; };
    };

    typedef multi_index<name("games"), games> games_table;

    ACTION creategame(name host, name opponent, int64_t bet_amount)
    {
        games_table _games_host(get_self(), host.value);
        games_table _games_opponent(get_self(), opponent.value);

        auto h_itr = _games_host.find(host.value);
        check(h_itr == _games_host.end(), "already exist");

        auto o_itr = _games_opponent.find(opponent.value);
        if (o_itr == _games_opponent.end())
        {
            _games_host.emplace(
                get_self(), [&](auto &newGame)
                {
            newGame.opponent = opponent;
            newGame.bet = bet_amount;
            newGame.status = false; });
        }
        else
        {
            check(!o_itr->status == true, "already accepted");
            check(o_itr->bet == bet_amount, "Bet amount not matched");
            _games_opponent.modify(
                o_itr, get_self(), [&](auto &updateGame)
                { updateGame.status = true; });
        }
    }

    [[eosio::on_notify("eosio.token::transfer ")]] ACTION create(name host, name _to, string opponentStr, asset amount)
    {
        auto opponent = name(opponentStr);
        // check(host != get_self(), "not allowed");
        if (host != get_self())
        {
            check(opponent != host && _to == get_self(), "Authorized User call this");
            creategame(host, opponent, amount.amount);
        }
    }

    ACTION close(name host, name opponent)
    {
        require_auth(host);

        games_table _games(get_self(), host.value);
        auto game_itr = _games.find(host.value);

        check(game_itr != _games.end(), "Game doesnt exist");
        check(game_itr->status == true, "game already accepted");

        double payout_amount = game_itr->bet;
        asset payout_asset(payout_amount, betting_token_symbol);

        action payout_action = action(
            permission_level{get_self(), "active"_n},
            "eosio.token"_n,
            "transfer"_n,
            std::make_tuple(get_self(), host, payout_asset, string("Sent")));
        payout_action.send();

        _games.erase(game_itr);
    }

    ACTION deleteall(name scope)
    {
        require_auth(get_self());
        games_table _games(get_self(), scope.value);

        for (auto itr = _games.begin();
             itr != _games.end();)
        {
            itr = _games.erase(itr);
        }
    }

private:
    const symbol betting_token_symbol;
};