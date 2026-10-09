using System;

namespace UnityEngine.Networking;

[Serializable]
[Obsolete("The UNET transport will be removed in the future as soon a replacement is ready.")]
public class ChannelQOS
{
	[SerializeField]
	internal QosType m_Type;

	[SerializeField]
	internal bool m_BelongsSharedOrderChannel;

	public QosType QOS => m_Type;

	public bool BelongsToSharedOrderChannel => m_BelongsSharedOrderChannel;

	public ChannelQOS(QosType value)
	{
		m_Type = value;
		m_BelongsSharedOrderChannel = false;
	}

	public ChannelQOS()
	{
		m_Type = QosType.Unreliable;
		m_BelongsSharedOrderChannel = false;
	}

	public ChannelQOS(ChannelQOS channel)
	{
		if (channel == null)
		{
			throw new NullReferenceException("channel is not defined");
		}
		m_Type = channel.m_Type;
		m_BelongsSharedOrderChannel = channel.m_BelongsSharedOrderChannel;
	}
}
