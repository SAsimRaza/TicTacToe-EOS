// tictactoe tutorial showing eosio smart contract

// Import the eosio base library.
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

// For convenience use the eosio namespace.
using namespace eosio;

// Declare the class.
// Use the [[eosio::contract(contract_name)]] attribute to let the
// compiler know this is a smart contract.
// Inherit from the eosio::contract public base class.
class [[eosio::contract("tictactoe")]] tictactoe : public contract
{
public:
    // Introduce base class members.
    using contract::contract;

    // Use the base class constructor.
    tictactoe(name receiver, name code, datastream<const char *> ds) : contract(receiver, code, ds), betting_token_symbol("ASA", 4)
    {
    }

    static constexpr name none = "none"_n;
    static constexpr name draw = "draw"_n;

    // Declare game data structure.
    struct [[eosio::table]] game
    {
        static constexpr uint16_t boardWidth = 3;
        static constexpr uint16_t boardHeight = boardWidth;

        game() : board(boardWidth * boardHeight, 0) {}

        name challenger;
        name host;
        name turn;          // = account name of host/ challenger
        name winner = none; // = none/ draw/ name of host/ name of challenger
        int64_t bet;
        bool status;
        std::vector<uint8_t> board;

        // Initialize board with empty cell
        void initializeBoard()
        {
            board.assign(boardWidth * boardHeight, 0);
        }

        // Reset game
        void resetGame()
        {
            initializeBoard();
            turn = host;
            winner = "none"_n;
        }

        auto primary_key() const { return challenger.value; }
        EOSLIB_SERIALIZE(game, (challenger)(host)(turn)(winner)(board))
    };

    // Define the games type which uses the game data structure.
    typedef eosio::multi_index<"games"_n, game> games;

    // Declare class method.
    [[eosio::on_notify("eosio.token::transfer ")]]
    [[eosio::action]] void create(name host, name _to, std::string opponentStr, asset amount);

    // Declare class method.
    [[eosio::action]] void creategame(name host, name challenger, int64_t bet_amount);

    // Declare class method.
    [[eosio::action]] void restart(const name &challenger, const name &host, const name &by);

    // Declare class method.
    [[eosio::action]] void close(const name &host, const name &challenger);

    // Declare class method.
    [[eosio::action]] void move(const name &challenger, const name &host, const name &by, const uint16_t &row, const uint16_t &column);

    // Declare class method.
    [[eosio::action]] void claimreward(name caller, name challenger);

    // Declare class method.
    [[eosio::action]] void deleteall(name scope);

private:
    const symbol betting_token_symbol;
    bool isEmptyCell(const uint8_t &cell);
    bool isValidMove(const uint16_t &row, const uint16_t &column, const std::vector<uint8_t> &board);
    name getWinner(const game &currentGame);
};