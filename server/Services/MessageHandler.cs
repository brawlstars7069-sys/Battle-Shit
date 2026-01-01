using System;
using System.Text.Json;
using System.Threading.Tasks;

namespace GameServer
{
	//обработчик входящих сообщений от клиентов
	public class MessageHandler : IMessageHandler
	{
		private readonly IPlayerService _playerService;
		private readonly IGameService _gameService;
		private readonly INetworkService _networkService;

		public MessageHandler(IPlayerService playerService, IGameService gameService, INetworkService networkService)
		{
			_playerService = playerService;
			_gameService = gameService;
			_networkService = networkService;
		}

		//главный метод обработки сообщений
		public async Task HandleMessage(string playerId, string json)
		{
			try
			{
				//парсим JSON в объект Message
				Message message = JsonSerializer.Deserialize<Message>(json);

				if (message == null)
				{
					Console.WriteLine($"[MessageHandler] Неверный формат сообщения от {playerId}");
					return;
				}

				//вызываем нужный метод в зависимости от действия
				switch (message.Action)
				{
					case "create_game":
						await HandleCreateGame(playerId, message.Data);
						break;

					case "get_games":
						await HandleGetGames(playerId);
						break;

					case "join_game":
						await HandleJoinGame(playerId, message.GameId);
						break;

					case "set_name":
						await HandleSetName(playerId, message.Data);
						break;

					case "ping":
						await HandlePing(playerId);
						break;

					default:
						Console.WriteLine($"[MessageHandler] Неизвестное действие: {message.Action}");
						await SendError(playerId, $"Неизвестное действие: {message.Action}");
						break;
				}
			}
			catch (Exception ex)
			{
				Console.WriteLine($"[MessageHandler] Ошибка обработки сообщения от {playerId}: {ex.Message}");
				await SendError(playerId, "Ошибка обработки сообщения");
			}
		}

		//обработка создания игры
		private async Task HandleCreateGame(string playerId, string gameName)
		{
			Console.WriteLine($"[MessageHandler] Игрок {playerId} создает игру: {gameName}");

			GameRoom game = _gameService.CreateGame(playerId, gameName);

			if (game == null)
			{
				await SendError(playerId, "Не удалось создать игру");
				return;
			}

			await _networkService.SendJson(playerId, new
			{
				action = "game_created",
				gameId = game.Id,
				gameName = game.Name
			});

			//уведомляем всех об обновлении списка игр
			await _networkService.Broadcast("games_updated");

			Console.WriteLine($"[MessageHandler] Игра создана: {game.Name} (ID: {game.Id})");
		}

		//обработка запроса списка игр
		private async Task HandleGetGames(string playerId)
		{
			Console.WriteLine($"[MessageHandler] Игрок {playerId} запрашивает список игр");

			var availableGames = _gameService.GetAvailableGames();

			var response = new
			{
				action = "games_list",
				games = availableGames
			};

			await _networkService.SendJson(playerId, response);

			Console.WriteLine($"[MessageHandler] Отправлен список игр игроку {playerId}");
		}

		//обработка присоединения к игре
		private async Task HandleJoinGame(string playerId, string gameId)
		{
			Console.WriteLine($"[MessageHandler] Игрок {playerId} присоединяется к игре {gameId}");

			bool success = _gameService.JoinGame(playerId, gameId);

			if (!success)
			{
				await SendError(playerId, "Не удалось присоединиться к игре");
				return;
			}

			GameRoom game = _gameService.GetGame(gameId);

			await _networkService.SendJson(game.Player1Id, new
			{
				action = "player_joined",
				opponentId = playerId,
				gameId = gameId
			});

			await _networkService.SendJson(playerId, new
			{
				action = "game_joined",
				opponentId = game.Player1Id,
				gameId = gameId,
				yourTurn = false
			});

			await _networkService.Broadcast("games_updated");

			Console.WriteLine($"[MessageHandler] Игрок {playerId} присоединился к игре {gameId}");
		}

		//обработка установки имени игрока
		private async Task HandleSetName(string playerId, string name)
		{
			Console.WriteLine($"[MessageHandler] Игрок {playerId} устанавливает имя: {name}");

			bool success = _playerService.UpdatePlayerName(playerId, name);

			if (success)
			{
				await _networkService.SendJson(playerId, new
				{
					action = "name_set",
					name = name
				});
				Console.WriteLine($"[MessageHandler] Имя установлено: {playerId} -> {name}");
			}
			else
			{
				await SendError(playerId, "Не удалось установить имя");
			}
		}

		//обработка ping запроса
		private async Task HandlePing(string playerId)
		{
			await _networkService.SendJson(playerId, new
			{
				action = "pong",
				timestamp = DateTime.UtcNow
			});

			Console.WriteLine($"[MessageHandler] Ping от игрока {playerId}");
		}

		//отправить сообщение об ошибке
		private async Task SendError(string playerId, string errorMessage)
		{
			await _networkService.SendJson(playerId, new
			{
				action = "error",
				message = errorMessage
			});

			Console.WriteLine($"[MessageHandler] Отправлена ошибка игроку {playerId}: {errorMessage}");
		}
	}
}