using System;
using System.Collections.Generic;
using System.Text;
using UnityEngine;

namespace Smooth.Compare.Utilities
{
	public class LogUnregisteredOnDestroy : MonoBehaviour
	{
		public bool destroyOnLoad;

		protected HashSet<Type> comparers = new HashSet<Type>();

		protected HashSet<Type> equalityComparers = new HashSet<Type>();

		protected void Awake()
		{
			if (!destroyOnLoad)
			{
				UnityEngine.Object.DontDestroyOnLoad(base.gameObject);
			}
			Finder.OnEvent.Handle += HandleFinderEvent;
		}

		protected void OnDestroy()
		{
			Finder.OnEvent.Handle -= HandleFinderEvent;
			if (comparers.Count <= 0 && equalityComparers.Count <= 0)
			{
				return;
			}
			StringBuilder stringBuilder = new StringBuilder();
			if (comparers.Count > 0)
			{
				stringBuilder.Append("Unregistered ").Append(ComparerType.Comparer.ToStringCached()).AppendLine("s :");
				foreach (Type comparer in comparers)
				{
					stringBuilder.AppendLine(comparer.FullName);
				}
			}
			if (equalityComparers.Count > 0)
			{
				if (stringBuilder.Length > 0)
				{
					stringBuilder.AppendLine();
				}
				stringBuilder.Append("Unregistered ").Append(ComparerType.EqualityComparer.ToStringCached()).AppendLine("s :");
				foreach (Type equalityComparer in equalityComparers)
				{
					stringBuilder.AppendLine(equalityComparer.FullName);
				}
			}
			Debug.Log(stringBuilder.ToString());
		}

		protected virtual void HandleFinderEvent(ComparerType comparerType, EventType eventType, Type type)
		{
			if (eventType == EventType.FindUnregistered && type.IsValueType)
			{
				switch (comparerType)
				{
				case ComparerType.EqualityComparer:
					equalityComparers.Add(type);
					break;
				case ComparerType.Comparer:
					comparers.Add(type);
					break;
				}
			}
		}
	}
}
