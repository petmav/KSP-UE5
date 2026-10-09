using System.Collections.Generic;
using UnityEngine;

public class ScenarioModule : MonoBehaviour, IConfigNode
{
	public ScenarioRunner runner;

	public ProtoScenarioModule snapshot;

	public List<GameScenes> targetScenes;

	[SerializeField]
	[HideInInspector]
	private string className;

	[HideInInspector]
	[SerializeField]
	private int classID;

	[SerializeField]
	[HideInInspector]
	private BaseEventList events;

	[SerializeField]
	[HideInInspector]
	private BaseFieldList fields;

	public string ClassName => className;

	public int ClassID => classID;

	public BaseEventList Events => events;

	public BaseFieldList Fields => fields;

	private void ModularSetup()
	{
		className = GetType().Name;
		classID = className.GetHashCode();
		events = new BaseEventList(this);
		fields = new BaseFieldList(this);
	}

	private void Awake()
	{
		targetScenes = new List<GameScenes>();
		ModularSetup();
		runner = GetComponent<ScenarioRunner>();
		OnAwake();
	}

	public void Save(ConfigNode node)
	{
		node.AddValue("name", ClassName);
		string text = "";
		int i = 0;
		for (int count = targetScenes.Count; i < count; i++)
		{
			text += (int)targetScenes[i];
			if (i < targetScenes.Count - 1)
			{
				text += ", ";
			}
		}
		node.AddValue("scene", text);
		Fields.Save(node);
		OnSave(node);
	}

	public void Load(ConfigNode node)
	{
		Fields.Load(node);
		OnLoad(node);
	}

	public virtual void OnAwake()
	{
	}

	public virtual void OnSave(ConfigNode node)
	{
	}

	public virtual void OnLoad(ConfigNode node)
	{
	}
}
