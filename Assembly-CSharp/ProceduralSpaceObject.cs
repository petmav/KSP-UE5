using System;
using System.Collections.Generic;
using UnityEngine;

public abstract class ProceduralSpaceObject : MonoBehaviour
{
	[Serializable]
	public class ModValue
	{
		public string name;

		public float minValue;

		public float maxValue;

		public float radiusFactor;
	}

	[Serializable]
	public class ModWrapper
	{
		public string name;

		public ModValue[] values;

		public PQSMod mod { get; set; }
	}

	[SerializeField]
	protected int seed;

	[SerializeField]
	public float radius;

	[SerializeField]
	protected SphereBaseSO visualSphere;

	[SerializeField]
	protected Material primaryMaterial;

	[SerializeField]
	protected Material secondaryMaterial;

	[SerializeField]
	protected string visualLayer;

	[SerializeField]
	protected string visualTag;

	[SerializeField]
	protected SphereBaseSO colliderSphere;

	[SerializeField]
	protected PhysicMaterial colliderMaterial;

	[SerializeField]
	protected string colliderLayer;

	[SerializeField]
	protected string colliderTag;

	[SerializeField]
	protected SphereBaseSO convexSphere;

	[SerializeField]
	protected PhysicMaterial convexMaterial;

	[SerializeField]
	protected string convexLayer;

	[SerializeField]
	protected string convexTag;

	[SerializeField]
	protected bool debugGenTime;

	[SerializeField]
	protected List<ModWrapper> mods;

	private void Reset()
	{
		NewSeed();
		UpdateWrappers();
		radius = 1f;
		visualLayer = "Default";
		visualTag = "Untagged";
		colliderLayer = "Default";
		colliderTag = "Untagged";
		convexLayer = "Default";
		convexTag = "Untagged";
	}

	protected Mesh CreateMeshVisual(SphereBaseSO sphere, float radius, List<PQSMod> modArray, out float volume, out float highestPoint)
	{
		Mod_OnSetup(modArray);
		PQS.VertexBuildData vertexBuildData = new PQS.VertexBuildData();
		float[] array = new float[sphere.vCount];
		Color[] array2 = new Color[sphere.vCount];
		volume = 4.18879f;
		highestPoint = radius;
		float num = 0f;
		for (int i = 0; i < sphere.vCount; i++)
		{
			vertexBuildData.vertHeight = radius;
			vertexBuildData.directionFromCenter = sphere.verts[i].position;
			vertexBuildData.vertColor = Color.white;
			Mod_Build(modArray, vertexBuildData);
			array[i] = (float)vertexBuildData.vertHeight;
			array2[i] = vertexBuildData.vertColor;
			num += array[i];
			if (array[i] > highestPoint)
			{
				highestPoint = array[i];
			}
		}
		if (num != 0f)
		{
			num /= (float)sphere.vCount;
			volume = GetSphereVolume(num);
		}
		return sphere.CreateMesh(array, array2, createUV: true);
	}

	protected Mesh CreateMeshCollider(SphereBaseSO sphere, float radius, List<PQSMod> modArray)
	{
		Mod_OnSetup(modArray);
		PQS.VertexBuildData vertexBuildData = new PQS.VertexBuildData();
		float[] array = new float[sphere.vCount];
		for (int i = 0; i < sphere.vCount; i++)
		{
			vertexBuildData.vertHeight = radius;
			vertexBuildData.directionFromCenter = sphere.verts[i].position;
			Mod_Build(modArray, vertexBuildData);
			array[i] = (float)vertexBuildData.vertHeight;
		}
		return sphere.CreateMesh(array, null, createUV: false);
	}

	private float GetSphereVolume(float r)
	{
		return 4.1887903f * r * r * r;
	}

	private void Mod_OnSetup(List<PQSMod> mods)
	{
		int i = 0;
		for (int count = mods.Count; i < count; i++)
		{
			mods[i].OnSetup();
		}
	}

	private void Mod_Build(List<PQSMod> mods, PQS.VertexBuildData vert)
	{
		int i = 0;
		for (int count = mods.Count; i < count; i++)
		{
			if (mods[i].modEnabled)
			{
				mods[i].OnVertexBuildHeight(vert);
			}
		}
		int j = 0;
		for (int count2 = mods.Count; j < count2; j++)
		{
			if (mods[j].modEnabled)
			{
				mods[j].OnVertexBuild(vert);
			}
		}
	}

	protected float RangefinderGeneric(Transform trf)
	{
		return Vector3.Distance(trf.position, Camera.main.transform.position);
	}

	[ContextMenu("Update Wrappers")]
	private void UpdateWrappers()
	{
		if (mods == null)
		{
			mods = new List<ModWrapper>();
		}
		int count = mods.Count;
		while (count-- > 0)
		{
			if ((PQSMod)GetComponent(mods[count].name) == null)
			{
				mods.RemoveAt(count);
			}
		}
		PQSMod[] components = GetComponents<PQSMod>();
		int i = 0;
		for (int num = components.Length; i < num; i++)
		{
			PQSMod pQSMod = components[i];
			if (!HasWrapperOfTypeName(pQSMod.GetType().Name))
			{
				ModWrapper modWrapper = new ModWrapper();
				modWrapper.name = pQSMod.GetType().Name;
				mods.Add(modWrapper);
			}
		}
	}

	private bool HasWrapperOfTypeName(string name)
	{
		int num = 0;
		int count = mods.Count;
		while (true)
		{
			if (num < count)
			{
				if (mods[num].name == name)
				{
					break;
				}
				num++;
				continue;
			}
			return false;
		}
		return true;
	}

	[ContextMenu("New Seed")]
	private void NewSeed()
	{
		seed = UnityEngine.Random.Range(int.MinValue, int.MaxValue);
	}
}
