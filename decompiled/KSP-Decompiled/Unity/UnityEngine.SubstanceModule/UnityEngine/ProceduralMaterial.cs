using System;
using System.ComponentModel;

namespace UnityEngine;

[Obsolete("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.", true)]
[ExcludeFromPreset]
[EditorBrowsable(EditorBrowsableState.Never)]
public sealed class ProceduralMaterial : Material
{
	public ProceduralCacheSize cacheSize
	{
		get
		{
			throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
		}
		set
		{
			FeatureRemoved();
		}
	}

	public int animationUpdateRate
	{
		get
		{
			throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
		}
		set
		{
			FeatureRemoved();
		}
	}

	public bool isProcessing
	{
		get
		{
			throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
		}
	}

	public bool isCachedDataAvailable
	{
		get
		{
			throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
		}
	}

	public bool isLoadTimeGenerated
	{
		get
		{
			throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
		}
		set
		{
			FeatureRemoved();
		}
	}

	public ProceduralLoadingBehavior loadingBehavior
	{
		get
		{
			throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
		}
	}

	public static bool isSupported
	{
		get
		{
			throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
		}
	}

	public static ProceduralProcessorUsage substanceProcessorUsage
	{
		get
		{
			throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
		}
		set
		{
			FeatureRemoved();
		}
	}

	public string preset
	{
		get
		{
			throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
		}
		set
		{
			FeatureRemoved();
		}
	}

	public bool isReadable
	{
		get
		{
			throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
		}
		set
		{
			FeatureRemoved();
		}
	}

	public bool isFrozen
	{
		get
		{
			throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
		}
	}

	private static void FeatureRemoved()
	{
		throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
	}

	internal ProceduralMaterial()
		: base((Material)null)
	{
		FeatureRemoved();
	}

	public ProceduralPropertyDescription[] GetProceduralPropertyDescriptions()
	{
		throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
	}

	public bool HasProceduralProperty(string inputName)
	{
		throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
	}

	public bool GetProceduralBoolean(string inputName)
	{
		throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
	}

	public bool IsProceduralPropertyVisible(string inputName)
	{
		throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
	}

	public void SetProceduralBoolean(string inputName, bool value)
	{
		FeatureRemoved();
	}

	public float GetProceduralFloat(string inputName)
	{
		throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
	}

	public void SetProceduralFloat(string inputName, float value)
	{
		FeatureRemoved();
	}

	public Vector4 GetProceduralVector(string inputName)
	{
		throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
	}

	public void SetProceduralVector(string inputName, Vector4 value)
	{
		FeatureRemoved();
	}

	public Color GetProceduralColor(string inputName)
	{
		throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
	}

	public void SetProceduralColor(string inputName, Color value)
	{
		FeatureRemoved();
	}

	public int GetProceduralEnum(string inputName)
	{
		throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
	}

	public void SetProceduralEnum(string inputName, int value)
	{
		FeatureRemoved();
	}

	public Texture2D GetProceduralTexture(string inputName)
	{
		throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
	}

	public void SetProceduralTexture(string inputName, Texture2D value)
	{
		FeatureRemoved();
	}

	public string GetProceduralString(string inputName)
	{
		throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
	}

	public void SetProceduralString(string inputName, string value)
	{
		FeatureRemoved();
	}

	public bool IsProceduralPropertyCached(string inputName)
	{
		throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
	}

	public void CacheProceduralProperty(string inputName, bool value)
	{
		FeatureRemoved();
	}

	public void ClearCache()
	{
		FeatureRemoved();
	}

	public void RebuildTextures()
	{
		FeatureRemoved();
	}

	public void RebuildTexturesImmediately()
	{
		FeatureRemoved();
	}

	public static void StopRebuilds()
	{
		FeatureRemoved();
	}

	public Texture[] GetGeneratedTextures()
	{
		throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
	}

	public ProceduralTexture GetGeneratedTexture(string textureName)
	{
		throw new Exception("Built-in support for Substance Designer materials has been removed from Unity. To continue using Substance Designer materials, you will need to install Allegorithmic's external importer from the Asset Store.");
	}

	public void FreezeAndReleaseSourceData()
	{
		FeatureRemoved();
	}
}
