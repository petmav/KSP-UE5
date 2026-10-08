using System;
using System.Collections.Generic;
using UnityEngine;

namespace Strategies
{
	[Serializable]
	public class DepartmentConfig
	{
		private string name;

		private string title;

		private string description;

		private GameObject avatarPrefab;

		private string headName;

		private string headImageString;

		private Texture2D headImage;

		private Color color;

		private List<StrategyConfig> strategies = new List<StrategyConfig>();

		public string Name => name;

		public string Title => title;

		public string Description => description;

		public GameObject AvatarPrefab => avatarPrefab;

		public string HeadName => headName;

		public string HeadImageString => headImageString;

		public Texture2D HeadImage => headImage;

		public Color Color => color;

		public List<StrategyConfig> Strategies => strategies;

		public static DepartmentConfig Create(ConfigNode node)
		{
			DepartmentConfig departmentConfig = new DepartmentConfig();
			string value = node.GetValue("name");
			if (string.IsNullOrEmpty(value))
			{
				return null;
			}
			departmentConfig.name = value;
			value = node.GetValue("title");
			if (!string.IsNullOrEmpty(value))
			{
				departmentConfig.title = value;
			}
			value = node.GetValue("desc");
			if (!string.IsNullOrEmpty(value))
			{
				departmentConfig.description = value;
			}
			value = node.GetValue("description");
			if (!string.IsNullOrEmpty(value))
			{
				departmentConfig.description = value;
			}
			value = node.GetValue("avatar");
			if (!string.IsNullOrEmpty(value))
			{
				departmentConfig.avatarPrefab = AssetBase.GetPrefab(value) ?? GameDatabase.Instance.GetModel(value);
			}
			value = node.GetValue("headName");
			if (!string.IsNullOrEmpty(value))
			{
				departmentConfig.headName = value;
			}
			value = node.GetValue("headImage");
			if (!string.IsNullOrEmpty(value))
			{
				departmentConfig.headImageString = value;
				departmentConfig.headImage = GameDatabase.Instance.GetTexture(departmentConfig.headImageString, asNormalMap: false);
			}
			value = node.GetValue("color");
			if (!string.IsNullOrEmpty(value))
			{
				departmentConfig.color = ConfigNode.ParseColor32(value);
			}
			return departmentConfig;
		}
	}
}
