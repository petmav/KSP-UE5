using System;
using System.ComponentModel;

namespace UnityEngine;

[Obsolete("MovieTexture is removed. Use VideoPlayer instead.", true)]
[EditorBrowsable(EditorBrowsableState.Never)]
[ExcludeFromObjectFactory]
[ExcludeFromPreset]
public sealed class MovieTexture : Texture
{
	[Obsolete("MovieTexture is removed. Use VideoPlayer instead.", true)]
	public AudioClip audioClip
	{
		get
		{
			FeatureRemoved();
			return null;
		}
	}

	[Obsolete("MovieTexture is removed. Use VideoPlayer instead.", true)]
	public bool loop
	{
		get
		{
			FeatureRemoved();
			return false;
		}
		set
		{
			FeatureRemoved();
		}
	}

	[Obsolete("MovieTexture is removed. Use VideoPlayer instead.", true)]
	public bool isPlaying
	{
		get
		{
			FeatureRemoved();
			return false;
		}
	}

	[Obsolete("MovieTexture is removed. Use VideoPlayer instead.", true)]
	public bool isReadyToPlay
	{
		get
		{
			FeatureRemoved();
			return false;
		}
	}

	[Obsolete("MovieTexture is removed. Use VideoPlayer instead.", true)]
	public float duration
	{
		get
		{
			FeatureRemoved();
			return 1f;
		}
	}

	private static void FeatureRemoved()
	{
		throw new Exception("MovieTexture has been removed from Unity. Use VideoPlayer instead.");
	}

	private MovieTexture()
	{
	}

	[Obsolete("MovieTexture is removed. Use VideoPlayer instead.", true)]
	public void Play()
	{
		FeatureRemoved();
	}

	[Obsolete("MovieTexture is removed. Use VideoPlayer instead.", true)]
	public void Stop()
	{
		FeatureRemoved();
	}

	[Obsolete("MovieTexture is removed. Use VideoPlayer instead.", true)]
	public void Pause()
	{
		FeatureRemoved();
	}
}
