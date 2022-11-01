#include <eosio/eosio.hpp>
using namespace eosio;
using namespace std;

CONTRACT tictactoe : public contract
{

public:
    using contract::contract;

    TABLE games
    {
        name host;
        name opponent;
        auto primary_key() const { return host.value; };
    };

    typedef multi_index<name("games"), games> games_table;

    ACTION create(name host, name opponent)
    {

        require_auth(host);

        games_table _games(host, host.value);

        auto itr = _games.find(host.value);

        if (itr == _games.end())
        {
            _games.emplace(
                host, [&](auto &newGame)
                {
                     newGame.host = host;
            newGame.opponent = opponent; });
        }
        else
        {
            _games.modify(
                itr, host, [&](auto &updateGame)
                {
                     updateGame.host = host;
            updateGame.opponent = opponent; });
        }
    }

    ACTION close()
    {
        // print("Hello,", host);
    }

private:
};