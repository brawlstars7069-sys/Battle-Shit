using System;
using System.Collections.Generic;
using System.Linq;

namespace GameServer
{
    //сервис дл€ работы с игровыми сервисами
    public class GameService : IGameService
    {
        private readonly Dictionary<string, GameRoom> _gameRooms = new Dictionary<string, GameRoom>();

        private readonly object _lock = new object();

        public GameRoom CreateGame(string playerId, string gameName)
        {
            lock (_lock)
            {
                if (IsPlayerInGame(playerId))
                {
                    Console.WriteLine($"[GameService] »грок {playerId} уже в игре, нельз€ создать новую");
                    return null;
                }

                GameRoom room = new GameRoom
                {
                    Name = gameName,
                    Player1Id = playerId,
                    Status = GameStatus.Waiting
                };

                _gameRooms[room.Id] = room;

                Console.WriteLine($"[GameService] —оздана нова€ игра: '{gameName}' (ID: {room.Id})");
                return room;
            }
        }

        public bool JoinGame(string playerId, string gameId)
        {
            lock (_lock)
            {
                if (IsPlayerInGame(playerId))
                {
                    Console.WriteLine($"[GameService] »грок {playerId} уже в игре, нельз€ присоединитьс€");
                    return false;
                }

                if (_gameRooms.TryGetValue(gameId, out GameRoom room))
                {
                    if (room.Status == GameStatus.Waiting && !room.IsFull)
                    {
                        room.Player2Id = playerId;
                        room.Status = GameStatus.PlacingShips;
                        room.StartedAt = DateTime.UtcNow;

                        Console.WriteLine($"[GameService] »грок {playerId} присоединилс€ к игре '{room.Name}'");
                        return true;
                    }
                    else
                    {
                        Console.WriteLine($"[GameService] »гра {gameId} недоступна дл€ присоединени€");
                        return false;
                    }
                }

                Console.WriteLine($"[GameService] »гра не найдена: {gameId}");
                return false;
            }
        }

        public List<GameRoom> GetAvailableGames()
        {
            lock (_lock)
            {
                List<GameRoom> availableGames = new List<GameRoom>();

                foreach (GameRoom room in _gameRooms.Values)
                {
                    if (room.Status == GameStatus.Waiting && !room.IsFull)
                    {
                        availableGames.Add(room);
                    }
                }

                return availableGames;
            }
        }

        public GameRoom GetGame(string gameId)
        {
            lock (_lock)
            {
                if (_gameRooms.TryGetValue(gameId, out GameRoom room))
                {
                    return room;
                }

                return null;
            }
        }

        public bool RemoveGame(string gameId)
        {
            lock (_lock)
            {
                bool removed = _gameRooms.Remove(gameId);

                if (removed)
                {
                    Console.WriteLine($"[GameService] »гра удалена: {gameId}");
                }
                else
                {
                    Console.WriteLine($"[GameService] »гра не найдена дл€ удалени€: {gameId}");
                }

                return removed;
            }
        }

        //проверить, находитс€ ли игрок в какой-либо игре
        public bool IsPlayerInGame(string playerId)
        {
            lock (_lock)
            {
                foreach (GameRoom room in _gameRooms.Values)
                {
                    if (room.HasPlayer(playerId))
                    {
                        return true; 
                    }
                }

                return false;
            }
        }

        //получить игру, в которой находитс€ игрок
        public GameRoom GetGameByPlayer(string playerId)
        {
            lock (_lock)
            {
                foreach (GameRoom room in _gameRooms.Values)
                {
                    if (room.HasPlayer(playerId))
                    {
                        return room; 
                    }
                }

                return null; 
            }
        }

        public List<GameRoom> GetAllGames()
        {
            lock (_lock)
            {
                List<GameRoom> allGames = new List<GameRoom>();
                foreach (GameRoom room in _gameRooms.Values)
                {
                    allGames.Add(room);
                }

                return allGames;
            }
        }

        //обновить статус игры
        public bool UpdateGameStatus(string gameId, GameStatus newStatus)
        {
            lock (_lock)
            {
                if (_gameRooms.TryGetValue(gameId, out GameRoom room))
                {
                    room.Status = newStatus;

                    //запоминаем врем€ завершени€
                    if (newStatus == GameStatus.Finished)
                    {
                        room.FinishedAt = DateTime.UtcNow;
                    }

                    Console.WriteLine($"[GameService] —татус игры {gameId} изменен на: {newStatus}");
                    return true;
                }

                return false;
            }
        }

        //обновить чей ход
        public bool UpdateCurrentTurn(string gameId, PlayerTurn turn)
        {
            lock (_lock)
            {
                if (_gameRooms.TryGetValue(gameId, out GameRoom room))
                {
                    // ќбновл€ем очередь хода
                    room.CurrentTurn = turn;
                    return true;
                }

                return false;
            }
        }

        //установить победител€ игры
        public bool SetWinner(string gameId, string winnerId)
        {
            lock (_lock)
            {
                if (_gameRooms.TryGetValue(gameId, out GameRoom room))
                {
                    room.WinnerId = winnerId;
                    room.Status = GameStatus.Finished;
                    room.FinishedAt = DateTime.UtcNow;

                    Console.WriteLine($"[GameService] ¬ игре {gameId} победил: {winnerId}");
                    return true;
                }

                return false;
            }
        }

        //€вл€етс€ ли игрок участником указанной игры
        public bool IsPlayerInGame(string playerId, string gameId)
        {
            lock (_lock)
            {
                if (_gameRooms.TryGetValue(gameId, out GameRoom room))
                {
                    return room.HasPlayer(playerId);
                }

                return false;
            }
        }

        public string GetOpponentId(string gameId, string playerId)
        {
            lock (_lock)
            {
                if (_gameRooms.TryGetValue(gameId, out GameRoom room))
                {
                    return room.GetOpponentId(playerId);
                }

                return null;
            }
        }

        public void CleanupOldGames(TimeSpan olderThan)
        {
            lock (_lock)
            {
                List<string> gamesToRemove = new List<string>();

                //вычисл€ем врем€ раньше которого игры считаютс€ старыми
                DateTime cutoffTime = DateTime.UtcNow - olderThan;

                foreach (var kvp in _gameRooms)
                {
                    string gameId = kvp.Key;
                    GameRoom room = kvp.Value;

                    //провер€ем услови€ игра завершена и врем€ завершени€ раньше порога
                    if (room.Status == GameStatus.Finished &&
                        room.FinishedAt < cutoffTime)
                    {
                        gamesToRemove.Add(gameId);
                    }
                }

                //удал€ем старые игры
                foreach (string gameId in gamesToRemove)
                {
                    _gameRooms.Remove(gameId);
                    Console.WriteLine($"[GameService] ќчищена стара€ игра: {gameId}");
                }

                //выводим статистику если что-то удалили
                if (gamesToRemove.Count > 0)
                {
                    Console.WriteLine($"[GameService] ќчищено {gamesToRemove.Count} старых игр");
                }
            }
        }

        //получить статистику по играм
        public void PrintStatistics()
        {
            lock (_lock)
            {
                int waitingCount = 0;
                int inProgressCount = 0;
                int finishedCount = 0;

                foreach (GameRoom room in _gameRooms.Values)
                {
                    if (room.Status == GameStatus.Waiting)
                    {
                        waitingCount++;
                    }
                    else if (room.Status == GameStatus.InProgress)
                    {
                        inProgressCount++;
                    }
                    else if (room.Status == GameStatus.Finished)
                    {
                        finishedCount++;
                    }
                }

                Console.WriteLine($"[GameService] —татистика: ќжидание={waitingCount}, ¬ процессе={inProgressCount}, «авершено={finishedCount}");
            }
        }
    }
}