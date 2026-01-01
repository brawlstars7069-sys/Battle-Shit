using System;

namespace GameServer
{
    //игровую комната (сессия игры)
    public class GameRoom
    {
        public string Id { get; set; }

        public string Name { get; set; }

        public string Player1Id { get; set; }

        public string Player2Id { get; set; }

        public GameStatus Status { get; set; }

        public PlayerTurn CurrentTurn { get; set; }

        public DateTime CreatedAt { get; private set; }

        public DateTime? StartedAt { get; set; }

        public DateTime? FinishedAt { get; set; }

        public string WinnerId { get; set; }

        public bool IsFull
        {
            get
            {
                //комната полная если оба id не пустые
                return !string.IsNullOrEmpty(Player1Id) && !string.IsNullOrEmpty(Player2Id);
            }
        }

        //проверка находится ли игрок в комнате
        public bool HasPlayer(string playerId)
        {
            return Player1Id == playerId || Player2Id == playerId;
        }

        public string GetOpponentId(string playerId)
        {
            if (Player1Id == playerId)
            {
                return Player2Id; 
            }
            else if (Player2Id == playerId)
            {
                return Player1Id; 
            }
            else
            {
                return null; 
            }
        }

        public GameRoom()
        {
            Id = Guid.NewGuid().ToString(); 
            Status = GameStatus.Waiting; 
            CurrentTurn = PlayerTurn.Player1; 
            CreatedAt = DateTime.UtcNow; 
        }
    }
}