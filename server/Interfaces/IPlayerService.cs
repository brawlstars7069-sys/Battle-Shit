using System.Collections.Generic;
using System.Net.Sockets;

namespace GameServer
{
    //интерфейс для работы с игроками
    public interface IPlayerService
    {
        Player AddPlayer(TcpClient client);
        bool RemovePlayer(string playerId);
        Player GetPlayer(string playerId);
        List<Player> GetConnectedPlayers();
        bool IsPlayerConnected(string playerId);
        int GetPlayerCount();
        bool UpdatePlayerName(string playerId, string newName);
    }
}