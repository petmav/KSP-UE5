using System;

namespace UnityEngine.Networking;

[Obsolete("The UNET transport will be removed in the future as soon a replacement is ready.")]
public class ConnectionSimulatorConfig : IDisposable
{
	internal int m_OutMinDelay;

	internal int m_OutAvgDelay;

	internal int m_InMinDelay;

	internal int m_InAvgDelay;

	internal float m_PacketLossPercentage;

	public ConnectionSimulatorConfig(int outMinDelay, int outAvgDelay, int inMinDelay, int inAvgDelay, float packetLossPercentage)
	{
		m_OutMinDelay = outMinDelay;
		m_OutAvgDelay = outAvgDelay;
		m_InMinDelay = inMinDelay;
		m_InAvgDelay = inAvgDelay;
		m_PacketLossPercentage = packetLossPercentage;
	}

	[ThreadAndSerializationSafe]
	public void Dispose()
	{
	}

	~ConnectionSimulatorConfig()
	{
		Dispose();
	}
}
