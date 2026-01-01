using System;

namespace GameServer
{
    //состояние игры
    public enum GameStatus
    {
        Waiting,
        PlacingShips,
        InProgress,
        Finished
    }
    //кто ходит
    public enum PlayerTurn
    {
        Player1,
        Player2
    }
    //результат выстрела
    public enum ShotResultType
    {
        Miss,
        Hit,
        Destroyed,
        Invalid
    }
}