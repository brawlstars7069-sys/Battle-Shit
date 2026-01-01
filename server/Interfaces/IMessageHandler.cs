using System.Threading.Tasks;

namespace GameServer
{
	//интерфейс для обработки входящих данных
	public interface IMessageHandler
	{
		//обработать данные от игрока
		Task HandleMessage(string playerId, string json);
	}
}