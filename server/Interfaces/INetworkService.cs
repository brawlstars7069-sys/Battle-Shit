using System.Threading.Tasks;

namespace GameServer
{
    //интерфейс для отправки данных
    public interface INetworkService
    {
        //конкретному игроку
        Task SendToClient(string playerId, string message);

        //всем игрокам (кроме указанных)
        Task Broadcast(string message, params string[] excludePlayerIds);

        //JSON конкретному игроку
        Task SendJson(string playerId, object data);

        //JSON нескольким игрокам
        Task SendJsonToPlayers(System.Collections.Generic.List<string> playerIds, object data);
    }
}