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

        static void InitializeServices()
        {
            Console.WriteLine("Инициализация сервисов...");


            playerService = new PlayerService();
            gameService = new GameService();
            networkService = new NetworkService(playerService);
            messageHandler = new MessageHandler(playerService, gameService, networkService);

            Console.WriteLine("NetworkService инициализирован");
            Console.WriteLine("GameService инициализирован");
            Console.WriteLine("PlayerService инициализирован");
            Console.WriteLine("MessageHandler инициализирован");
            Console.WriteLine("Сервисы инициализированы!");
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
    }
}