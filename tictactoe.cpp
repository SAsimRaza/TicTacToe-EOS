#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
using namespace eosio;
using namespace std;

CONTRACT tictactoe : public contract
{

public:
    using contract::contract;

    tictactoe(name reciever, name code, datastream<const char *> ds) : contract(reciever, code, ds), bettoken_symbol("ASA", 4)
    {
    }

    TABLE games
    {
        name host;
        name opponent;
        int64_t bet;
        bool status;
        auto primary_key() const { return host.value; };
    };

    typedef multi_index<name("games"), games> games_table;

    void create_game(name host, name opponent, int64_t bet_amount)
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
            newGame.host = host;
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
        check(host != get_self(), "not allowed");
        check(amount.symbol == bettoken_symbol && amount.amount > 0, "IV");
        check(opponent != host && _to == get_self(), "Authorized User call this");
        create_game(host, opponent, amount.amount);
    }

    ACTION close()
    {
        // print("Hello,", host);
    }

private:
    const symbol bettoken_symbol;
};