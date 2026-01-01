using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using System.Text.Json;

namespace GameServer
{
    class Program
    {
        //Сервисы
        private static IPlayerService playerService;
        private static IGameService gameService;
        private static INetworkService networkService;
        private static IMessageHandler messageHandler;

        //Настройки сервера
        private static string serverIp;
        private static int serverPort;

        static async Task Main(string[] args)
        {
            Console.WriteLine("====================================");
            Console.WriteLine("    Сервер Морской Бой (Battle Shit)");
            Console.WriteLine("====================================");
            Console.WriteLine();

            //Получаем настройки от пользователя
            GetServerSettings();

            //Создаем экземпляры сервисов (пока заглушки)
            InitializeServices();

            //Запускаем сервер
            await StartServer();
        }

        static void GetServerSettings()
        {
            Console.WriteLine("Настройка сервера:");
            Console.WriteLine("------------------");

            Console.Write("Введите IP адрес сервера (например: 26.1.1.1): ");
            serverIp = Console.ReadLine();

            Console.Write("Введите порт сервера (например: 8888): ");
            serverPort = int.Parse(Console.ReadLine());

            Console.WriteLine($"Сервер будет запущен на {serverIp}:{serverPort}");
            Console.WriteLine();
        }

        //создаем все необходимые сервисы (пока заглушки)
        static void InitializeServices()
        {
            Console.WriteLine("Инициализация сервисов...");

            //В следующих коммитах будут полноценные классы

            playerService = new PlayerServiceStub();
            gameService = new GameServiceStub();
            networkService = new NetworkServiceStub();
            messageHandler = new MessageHandlerStub();

            Console.WriteLine("GameService инициализирован");
            Console.WriteLine("PlayerService инициализирован");
            Console.WriteLine("Сервисы инициализированы (заглушка)");
            Console.WriteLine();
        }

        static async Task StartServer()
        {
            try
            {
                //слушатель подключений
                TcpListener server = new TcpListener(IPAddress.Parse(serverIp), serverPort);
                server.Start();

                Console.WriteLine("====================================");
                Console.WriteLine($"Сервер запущен и слушает на {serverIp}:{serverPort}");
                Console.WriteLine("Ожидание подключений клиентов...");
                Console.WriteLine("====================================");
                Console.WriteLine();

                //цикл принятия подключений
                while (true)
                {

                    TcpClient client = await server.AcceptTcpClientAsync();
                    _ = Task.Run(() => HandleClientConnection(client));
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"ОШИБКА: Не удалось запустить сервер");
                Console.WriteLine($"Подробности: {ex.Message}");
                Console.WriteLine("Нажмите любую клавишу для выхода...");
                Console.ReadKey();
            }
        }

        //обработка подключения клиента
        static async Task HandleClientConnection(TcpClient client)
        {
            Player player = null;

            try
            {
                Console.WriteLine("Новое подключение обнаружено!");

                //Добавляем игрока в систему
                player = playerService.AddPlayer(client);
                Console.WriteLine($"Игрок зарегистрирован: {player.Id}");

                //Получаем сетевой поток для обмена данными
                NetworkStream stream = client.GetStream();
                byte[] buffer = new byte[4096]; // Буфер для приема данных

                //Цикл приема сообщений от клиента
                while (client.Connected)
                {
                    //Читаем данные из потока
                    int bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length);

                    //Если данных нет - соединение разорвано
                    if (bytesRead == 0)
                    {
                        Console.WriteLine($"Клиент {player.Id} отключился (пустое сообщение)");
                        break;
                    }

                    //Преобразуем байты в строку
                    string message = Encoding.UTF8.GetString(buffer, 0, bytesRead);
                    Console.WriteLine($"Получено от {player.Id}: {message}");

                    //Передаем сообщение на обработку
                    await messageHandler.HandleMessage(player.Id, message);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Ошибка обработки клиента {player?.Id}: {ex.Message}");
            }
            finally
            {
                //Очищаем ресурсы при отключении
                if (player != null)
                {
                    playerService.RemovePlayer(player.Id);
                    Console.WriteLine($"Игрок удален из системы: {player.Id}");
                }

                //Закрываем соединение
                client.Close();
                Console.WriteLine($"Соединение закрыто");
            }
        }

        //Временные заглушки классов

        class PlayerServiceStub : IPlayerService
        {
            public Player AddPlayer(TcpClient client)
            {
                Console.WriteLine("PlayerServiceStub: Добавлен новый игрок (заглушка)");
                return new Player { Client = client };
            }

            public bool RemovePlayer(string playerId)
            {
                Console.WriteLine($"PlayerServiceStub: Удален игрок {playerId} (заглушка)");
                return true;
            }

            public Player GetPlayer(string playerId)
            {
                Console.WriteLine($"PlayerServiceStub: Получен игрок {playerId} (заглушка)");
                return new Player { Id = playerId };
            }

            public List<Player> GetConnectedPlayers()
            {
                Console.WriteLine("PlayerServiceStub: Получен список игроков (заглушка)");
                return new List<Player>();
            }

            public bool IsPlayerConnected(string playerId)
            {
                Console.WriteLine($"PlayerServiceStub: Проверка подключения {playerId} (заглушка)");
                return true;
            }

            public int GetPlayerCount()
            {
                Console.WriteLine("PlayerServiceStub: Получено количество игроков (заглушка)");
                return 0;
            }
        }

        class GameServiceStub : IGameService
        {
            public GameRoom CreateGame(string playerId, string gameName)
            {
                Console.WriteLine($"GameServiceStub: Создана игра '{gameName}' (заглушка)");
                return new GameRoom { Name = gameName, Player1Id = playerId };
            }

            public bool JoinGame(string playerId, string gameId)
            {
                Console.WriteLine($"GameServiceStub: Игрок {playerId} присоединился к игре {gameId} (заглушка)");
                return true;
            }

            public List<GameRoom> GetAvailableGames()
            {
                Console.WriteLine("GameServiceStub: Получен список игр (заглушка)");
                return new List<GameRoom>();
            }

            public GameRoom GetGame(string gameId)
            {
                Console.WriteLine($"GameServiceStub: Получена игра {gameId} (заглушка)");
                return new GameRoom { Id = gameId };
            }

            public bool RemoveGame(string gameId)
            {
                Console.WriteLine($"GameServiceStub: Удалена игра {gameId} (заглушка)");
                return true;
            }

            public bool IsPlayerInGame(string playerId)
            {
                Console.WriteLine($"GameServiceStub: Проверка игрока в игре {playerId} (заглушка)");
                return false;
            }

            public GameRoom GetGameByPlayer(string playerId)
            {
                Console.WriteLine($"GameServiceStub: Получена игра игрока {playerId} (заглушка)");
                return null;
            }
        }

        class NetworkServiceStub : INetworkService
        {
            public Task SendToClient(string playerId, string message)
            {
                Console.WriteLine($"NetworkServiceStub: Отправлено игроку {playerId}: {message}");
                return Task.CompletedTask;
            }

            public Task Broadcast(string message, params string[] excludePlayerIds)
            {
                Console.WriteLine($"NetworkServiceStub: Рассылка всем: {message}");
                return Task.CompletedTask;
            }

            public Task SendJson(string playerId, object data)
            {
                string json = JsonSerializer.Serialize(data);
                Console.WriteLine($"NetworkServiceStub: Отправлен JSON игроку {playerId}: {json}");
                return Task.CompletedTask;
            }

            public Task SendJsonToPlayers(List<string> playerIds, object data)
            {
                string json = JsonSerializer.Serialize(data);
                Console.WriteLine($"NetworkServiceStub: Отправлен JSON игрокам: {json}");
                return Task.CompletedTask;
            }
        }

        class MessageHandlerStub : IMessageHandler
        {
            public Task HandleMessage(string playerId, string json)
            {
                Console.WriteLine($"MessageHandlerStub: Обработка сообщения от {playerId}: {json}");
                return Task.CompletedTask;
            }
        }
    }
}