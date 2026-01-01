using System.Collections.Generic;

namespace GameServer
{
	//интерфейс для работы с игровыми сессиями
	public interface IGameService
	{
		GameRoom CreateGame(string playerId, string gameName);
		bool JoinGame(string playerId, string gameId);
		List<GameRoom> GetAvailableGames();
		GameRoom GetGame(string gameId);
		bool RemoveGame(string gameId);
		bool IsPlayerInGame(string playerId);
		GameRoom GetGameByPlayer(string playerId);
	}
}