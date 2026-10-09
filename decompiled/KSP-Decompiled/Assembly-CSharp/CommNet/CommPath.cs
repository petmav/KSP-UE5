using CommNet.Network;

namespace CommNet
{
	public class CommPath : Path<CommNetwork, CommNode, CommLink, CommPath>
	{
		protected CommLink first;

		protected CommLink last;

		public VesselControlState commType { get; protected set; }

		public double signalStrength { get; protected set; }

		public SignalStrength signal { get; protected set; }

		public override CommLink First => first;

		public override CommLink Last => last;

		public bool IsLastHopHome()
		{
			return Last.b.isHome;
		}

		public new virtual void Clear()
		{
			base.Clear();
			commType = VesselControlState.None;
			signalStrength = 0.0;
			signal = SignalStrength.None;
			first = null;
			last = null;
		}

		public override void UpdateFromPath()
		{
			int count = base.Count;
			if (count < 1)
			{
				Clear();
				return;
			}
			first = base[0];
			last = base[count - 1];
			signalStrength = 1.0;
			for (int i = 0; i < count; i++)
			{
				CommLink commLink = base[i];
				signalStrength *= commLink.signalStrength;
			}
			last.hopType = ((!last.b.isHome) ? HopType.ControlPoint : HopType.Home);
			signal = NodeUtilities.ConvertSignalStrength(signalStrength);
		}

		public override string ToString()
		{
			if (base.Count == 0)
			{
				return string.Empty;
			}
			string text = base[0].start.name;
			int i = 0;
			for (int count = base.Count; i < count; i++)
			{
				text = text + ";" + base[i].end.name;
			}
			return text;
		}
	}
}
