using System;
using System.Collections.Generic;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

namespace GameServer
{
	//сервис для отправки сообщений по сети
	public class NetworkService : INetworkService
	{
		private readonly IPlayerService _playerService;

		public NetworkService(IPlayerService playerService)
		{
			_playerService = playerService;
		}

		public async Task SendToClient(string playerId, string message)
		{
			Player player = _playerService.GetPlayer(playerId);

			if (player == null)
			{
				Console.WriteLine($"[NetworkService] Игрок не найден: {playerId}");
				return;
			}

			if (!player.IsConnected)
			{
				Console.WriteLine($"[NetworkService] Игрок отключен: {playerId}");
				return;
			}

			try
			{
				//получаем сетевой поток
				NetworkStream stream = player.Client.GetStream();

				//преобразуем сообщение в байты
				byte[] data = Encoding.UTF8.GetBytes(message);

				//отправляем данные
				await stream.WriteAsync(data, 0, data.Length);

				//очищаем буфер
				await stream.FlushAsync();

				Console.WriteLine($"[NetworkService] Отправлено {playerId}: {message}");
			}
			catch (Exception ex)
			{
				Console.WriteLine($"[NetworkService] Ошибка отправки {playerId}: {ex.Message}");
			}
		}

		//отправить сообщение всем игрокам
		public async Task Broadcast(string message, params string[] excludePlayerIds)
		{
			List<Player> players = _playerService.GetConnectedPlayers();
			List<Task> sendTasks = new List<Task>();

			HashSet<string> excludeSet = new HashSet<string>();
			foreach (string excludedId in excludePlayerIds)
			{
				excludeSet.Add(excludedId);
			}

			//для каждого игрока создаем задачу отправки
			foreach (Player player in players)
			{
				//пропускаем исключенных игроков
				if (excludeSet.Contains(player.Id))
				{
					continue;
				}

				//добавляем задачу отправки
				sendTasks.Add(SendToClient(player.Id, message));
			}

			if (sendTasks.Count > 0)
			{
				await Task.WhenAll(sendTasks);
				Console.WriteLine($"[NetworkService] Рассылка завершена: {sendTasks.Count} игрокам");
			}
		}

		//отправить JSON сообщение
		public async Task SendJson(string playerId, object data)
		{
			//преобразуем объект в JSON строку
			string json = JsonSerializer.Serialize(data);
			await SendToClient(playerId, json);
		}

		//отправить JSON нескольким игрокам
		public async Task SendJsonToPlayers(List<string> playerIds, object data)
		{
			string json = JsonSerializer.Serialize(data);
			List<Task> sendTasks = new List<Task>();

			//для каждого игрока создаем задачу отправки
			foreach (string playerId in playerIds)
			{
				sendTasks.Add(SendToClient(playerId, json));
			}

			//ждем завершения всех отправок
			if (sendTasks.Count > 0)
			{
				await Task.WhenAll(sendTasks);
			}
		}

		//отправить сообщение двум игрокам в игре
		public async Task SendToGamePlayers(string gameId, object data, IGameService gameService)
		{ 
			GameRoom game = gameService.GetGame(gameId);
			if (game == null)
			{
				Console.WriteLine($"[NetworkService] Игра не найдена: {gameId}");
				return;
			}

			List<string> playerIds = new List<string>();
			if (!string.IsNullOrEmpty(game.Player1Id))
			{
				playerIds.Add(game.Player1Id);
			}

			if (!string.IsNullOrEmpty(game.Player2Id))
			{
				playerIds.Add(game.Player2Id);
			}

			//отправляем всем игрокам игры
			await SendJsonToPlayers(playerIds, data);
		}
	}
}