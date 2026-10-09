using System;
using System.Collections.Generic;
using UnityEngine;

namespace KSP.UI
{
	[RequireComponent(typeof(CanvasGroup))]
	public class CanvasGroupInputLock : MonoBehaviour
	{
		private static bool disableAllLocking;

		[SerializeField]
		private List<string> inputLockMask = new List<string>();

		private List<string> defaultMask = new List<string>();

		private ulong lockMask;

		private bool lockMaskDirty = true;

		private CanvasGroup canvasGroup;

		public List<string> InputLockMask
		{
			get
			{
				lockMaskDirty = true;
				return inputLockMask;
			}
		}

		public ulong LockMask
		{
			get
			{
				return lockMask;
			}
			set
			{
				lockMask = value;
				lockMaskDirty = false;
			}
		}

		private void UpdateLockMask()
		{
			lockMask = 0uL;
			int i = 0;
			for (int count = inputLockMask.Count; i < count; i++)
			{
				try
				{
					ControlTypes controlTypes = (ControlTypes)Enum.Parse(typeof(ControlTypes), inputLockMask[i], ignoreCase: true);
					lockMask |= (ulong)controlTypes;
				}
				catch (Exception ex)
				{
					Debug.Log("CanvasGroupInputLock '" + base.gameObject.name + "': " + ex.Message);
				}
			}
			Debug.Log(base.gameObject.name + " MASK: " + lockMask);
			lockMaskDirty = false;
		}

		private void Awake()
		{
			canvasGroup = GetComponent<CanvasGroup>();
			InitializeDefaultMask();
			GameEvents.onInputLocksModified.Add(OnInputLocksModified);
		}

		private void InitializeDefaultMask()
		{
			defaultMask.Clear();
			for (int i = 0; i < inputLockMask.Count; i++)
			{
				defaultMask.Add(inputLockMask[i]);
			}
		}

		public void SetInputMaskToDefault()
		{
			SetInputMask(defaultMask);
		}

		public void SetInputMask(List<string> newMask)
		{
			inputLockMask.Clear();
			for (int i = 0; i < newMask.Count; i++)
			{
				inputLockMask.Add(newMask[i]);
			}
			UpdateLockMask();
		}

		private void OnDestroy()
		{
			GameEvents.onInputLocksModified.Remove(OnInputLocksModified);
		}

		private void Start()
		{
			if (!disableAllLocking)
			{
				UpdateLockMask();
				canvasGroup.blocksRaycasts = (InputLockManager.lockMask & lockMask) == 0L;
			}
		}

		private void OnInputLocksModified(GameEvents.FromToAction<ControlTypes, ControlTypes> fromTo)
		{
			if (!disableAllLocking)
			{
				if (lockMaskDirty)
				{
					UpdateLockMask();
				}
				canvasGroup.blocksRaycasts = ((ulong)fromTo.to & lockMask) == 0L;
			}
		}

		public static string ToBits(ControlTypes lockMask)
		{
			return ToBits((ulong)lockMask);
		}

		public static string ToBits(ulong lockMask)
		{
			string text = Convert.ToString((long)lockMask, 2);
			if (text.Length < 64)
			{
				text = new string('0', 64 - text.Length) + text;
			}
			return text;
		}
	}
}
