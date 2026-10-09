using System;
using UnityEngine;

[Serializable]
public class PartRawInfo
{
	[SerializeField]
	private string name;

	[SerializeField]
	private string value;

	public string Name => name;

	public string Value => value;

	public PartRawInfo(string name, string value)
	{
		this.name = name;
		this.value = value;
	}
}
