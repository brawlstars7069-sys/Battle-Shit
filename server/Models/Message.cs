using System.Text.Json.Serialization;
//обмен между клиентом и сервером
namespace GameServer
{

    public class Message
    {
        //тип действия
        [JsonPropertyName("action")] 
        public string Action { get; set; }

        //данные действия
        [JsonPropertyName("data")] 
        public string Data { get; set; }

        //id игры
        [JsonPropertyName("gameId")] 
        public string GameId { get; set; }
    }
}