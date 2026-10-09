using System;

public class GameBackup
{
	public double UniversalTime { get; private set; }

	public ConfigNode Config { get; private set; }

	public int ActiveVessel { get; private set; }

	public Guid ActiveVesselID { get; private set; }

	public GameBackup(Game game)
	{
		UniversalTime = game.UniversalTime;
		ActiveVessel = game.flightState.activeVesselIdx;
		if (ActiveVessel >= 0 && ActiveVessel < game.flightState.protoVessels.Count)
		{
			ActiveVesselID = game.flightState.protoVessels[game.flightState.activeVesselIdx].vesselID;
		}
		else
		{
			ActiveVessel = 0;
			ActiveVesselID = Guid.Empty;
		}
		Config = new ConfigNode();
		game.Save(Config);
	}
}
