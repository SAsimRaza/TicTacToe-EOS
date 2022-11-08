#include "tictactoe.hpp"

void tictactoe::creategame(name host, name challenger, int64_t bet_amount)
{
    games _games_host(get_self(), host.value);
    games _games_opponent(get_self(), challenger.value);

    auto h_itr = _games_host.find(host.value);
    check(h_itr == _games_host.end(), "already exist");

    auto o_itr = _games_opponent.find(challenger.value);
    if (o_itr == _games_opponent.end())
    {
        _games_host.emplace(
            get_self(), [&](auto &newGame)
            {
                    newGame.host = host;
            newGame.challenger = challenger;
            newGame.bet = bet_amount;
            newGame.status = false;
            newGame.turn = host; });
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

// [[eosio::on_notify("eosio.token::transfer ")]]
void tictactoe::create(name host, name _to, std::string opponentStr, asset amount)
{
    print("Notify call from Create");
    auto opponent = name(opponentStr);
    // check(host != get_self(), "not allowed");
    check()
    if (host != get_self())
    {
        check(opponent != host && _to == get_self(), "Authorized User call this");
        creategame(host, opponent, amount.amount);
    }
}

void tictactoe::close(const name &host, const name &challenger)
{
    require_auth(host);

    games _games(get_self(), host.value);
    auto game_itr = _games.find(host.value);

    check(game_itr != _games.end(), "Game doesnt exist");
    check(game_itr->status == true, "game already accepted");

    double payout_amount = game_itr->bet;
    asset payout_asset(payout_amount, betting_token_symbol);

    action payout_action = action(
        permission_level{get_self(), "active"_n},
        "eosio.token"_n,
        "transfer"_n,
        std::make_tuple(get_self(), host, payout_asset, std::string("Sent")));
    payout_action.send();

    _games.erase(game_itr);
}

void tictactoe::restart(const name &challenger, const name &host, const name &by)
{
    check(has_auth(by), "Only " + by.to_string() + "can restart the game.");

    // Check if game exists
    games existingHostGames(get_self(), host.value);
    auto itr = existingHostGames.find(challenger.value);
    check(itr != existingHostGames.end(), "Game does not exist.");

    // Check if this game belongs to the action sender
    check(by == itr->host || by == itr->challenger, "This is not your game.");

    // Reset game
    existingHostGames.modify(itr, itr->host, [](auto &g)
                             { g.resetGame(); });
}

void tictactoe::move(const name &challenger, const name &host, const name &by, const uint16_t &row, const uint16_t &column)
{
    check(has_auth(by), "The next move should be made by " + by.to_string());

    // Check if game exists
    games existingHostGames(get_self(), host.value);
    auto itr = existingHostGames.find(challenger.value);
    check(itr != existingHostGames.end(), "Game does not exist.");

    // Check if this game hasn't ended yet
    check(itr->winner == none, "The game has ended.");

    // Check if this game belongs to the action sender
    check(by == itr->host || by == itr->challenger, "This is not your game.");
    // Check if this is the  action sender's turn
    check(by == itr->turn, "it's not your turn yet!");

    // Check if user makes a valid movement
    check(isValidMove(row, column, itr->board), "Not a valid movement.");

    // Fill the cell, 1 for host, 2 for challenger
    // TODO could use constant for 1 and 2 as well
    const uint8_t cellValue = itr->turn == itr->host ? 1 : 2;
    const auto turn = itr->turn == itr->host ? itr->challenger : itr->host;
    existingHostGames.modify(itr, itr->host, [&](auto &g)
                             {
        g.board[row * game::boardWidth + column] = cellValue;
        g.turn = turn;
        g.winner = getWinner(g); });
}

void tictactoe::claimreward(name caller, name challenger)
{
    require_auth(caller);

    games existingHostGames(get_self(), challenger.value);
    auto itr = existingHostGames.find(challenger.value);
    check(itr != existingHostGames.end(), "Game does not exist.");
    check(bool(itr->host == caller) || bool(itr->challenger == caller), "Unauthorize");

    name winner = itr->winner;
    check(winner != none, "Winner not decided yet");
    double payout_amount = itr->bet;
    asset payout_asset(payout_amount, betting_token_symbol);

    action{
        permission_level{get_self(), "active"_n},
        "eosio.token"_n,
        "transfer"_n,
        std::make_tuple(get_self(), winner, payout_asset, std::string("Sent!"))}
        .send();

    existingHostGames.erase(itr);
}

void tictactoe::deleteall(name scope)
{
    require_auth(get_self());
    games _games(get_self(), scope.value);

    for (auto itr = _games.begin();
         itr != _games.end();)
    {
        itr = _games.erase(itr);
    }
}

bool tictactoe::isEmptyCell(const uint8_t &cell)
{
    return cell == 0;
}

bool tictactoe::isValidMove(const uint16_t &row, const uint16_t &column, const std::vector<uint8_t> &board)
{
    uint32_t movementLocation = row * game::boardWidth + column;
    bool isValid = movementLocation < board.size() && isEmptyCell(board[movementLocation]);
    return isValid;
}

name tictactoe::getWinner(const game &currentGame)
{
    auto &board = currentGame.board;

    bool isBoardFull = true;

    // Use bitwise AND operator to determine the consecutive values of each column, row and diagonal
    // Since 3 == 0b11, 2 == 0b10, 1 = 0b01, 0 = 0b00
    std::vector<uint32_t> consecutiveColumn(game::boardWidth, 3);
    std::vector<uint32_t> consecutiveRow(game::boardHeight, 3);
    uint32_t consecutiveDiagonalBackslash = 3;
    uint32_t consecutiveDiagonalSlash = 3;

    for (uint32_t i = 0; i < board.size(); i++)
    {
        isBoardFull &= isEmptyCell(board[i]);
        uint16_t row = uint16_t(i / game::boardWidth);
        uint16_t column = uint16_t(i % game::boardWidth);

        // Calculate consecutive row and column value
        consecutiveRow[column] = consecutiveRow[column] & board[i];
        consecutiveColumn[row] = consecutiveColumn[row] & board[i];
        // Calculate consecutive diagonal \ value
        if (row == column)
        {
            consecutiveDiagonalBackslash = consecutiveDiagonalBackslash & board[i];
        }
        // Calculate consecutive diagonal / value
        if (row + column == game::boardWidth - 1)
        {
            consecutiveDiagonalSlash = consecutiveDiagonalSlash & board[i];
        }
    }

    // Inspect the value of all consecutive row, column, and diagonal and determine winner
    std::vector<uint32_t> aggregate = {consecutiveDiagonalBackslash, consecutiveDiagonalSlash};
    aggregate.insert(aggregate.end(), consecutiveColumn.begin(), consecutiveColumn.end());
    aggregate.insert(aggregate.end(), consecutiveRow.begin(), consecutiveRow.end());

    for (auto value : aggregate)
    {
        if (value == 1)
        {
            return currentGame.host;
        }
        else if (value == 2)
        {
            return currentGame.challenger;
        }
    }
    // Draw if the board is full, otherwise the winner is not determined yet
    return isBoardFull ? draw : none;
}
