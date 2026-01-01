using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;

namespace GameServer
{
	//сервис для работы с игроками
	public class PlayerService : IPlayerService
	{
		//словарь для поиска по id
		private readonly Dictionary<string, Player> _players = new Dictionary<string, Player>();

		//чтобы избежать проблем при многопоточном доступе
		private readonly object _lock = new object();

		public Player AddPlayer(TcpClient client)
		{
			lock (_lock)
			{
				Player player = new Player
				{
					Client = client
				};

				//cохраняем в словаре
				_players[player.Id] = player;

				Console.WriteLine($"[PlayerService] Добавлен новый игрок: {player.Id}");
				return player;
			}
		}

		public bool RemovePlayer(string playerId)
		{
			lock (_lock)
			{
				if (_players.ContainsKey(playerId))
				{
					Player player = _players[playerId];

					if (player.Client != null && player.Client.Connected)
					{
						player.Client.Close();
					}
					//удаляем из словаря
					_players.Remove(playerId);

					Console.WriteLine($"[PlayerService] Игрок удален: {playerId}");
					return true;
				}

				Console.WriteLine($"[PlayerService] Игрок не найден: {playerId}");
				return false;
			}
		}

		public Player GetPlayer(string playerId)
		{
			lock (_lock)
			{
				if (_players.TryGetValue(playerId, out Player player))
				{
					return player;
				}

				return null;
			}
		}

		public List<Player> GetConnectedPlayers()
		{
			lock (_lock)
			{
				//фильтруем только тех, кто подключен
				return _players.Values
					.Where(player => player.IsConnected)
					.ToList();
			}
		}

		public bool IsPlayerConnected(string playerId)
		{
			lock (_lock)
			{
				if (_players.TryGetValue(playerId, out Player player))
				{
					return player.IsConnected;
				}

				return false;
			}
		}
		public int GetPlayerCount()
		{
			lock (_lock)
			{
				return _players.Count;
			}
		}

		public List<Player> GetAllPlayers()
		{
			lock (_lock)
			{
				return _players.Values.ToList();
			}
		}

		public bool UpdatePlayerName(string playerId, string newName)
		{
			lock (_lock)
			{
				if (_players.TryGetValue(playerId, out Player player))
				{
					player.Name = newName;
					Console.WriteLine($"[PlayerService] Игрок {playerId} сменил имя на: {newName}");
					return true;
				}

				return false;
			}
		}

		public bool PlayerExists(string playerId)
		{
			lock (_lock)
			{
				return _players.ContainsKey(playerId);
			}
		}

		///очистить всех отключенных игроков
		public void CleanupDisconnectedPlayers()
		{
			lock (_lock)
			{
				List<string> disconnectedIds = new List<string>();

				//находим всех отключенных игроков
				foreach (var kvp in _players)
				{
					if (!kvp.Value.IsConnected)
					{
						disconnectedIds.Add(kvp.Key);
					}
				}

				//удаляем их
				foreach (string id in disconnectedIds)
				{
					_players.Remove(id);
					Console.WriteLine($"[PlayerService] Очищен отключенный игрок: {id}");
				}

				if (disconnectedIds.Count > 0)
				{
					Console.WriteLine($"[PlayerService] Очищено {disconnectedIds.Count} отключенных игроков");
				}
			}
		}
	}
}