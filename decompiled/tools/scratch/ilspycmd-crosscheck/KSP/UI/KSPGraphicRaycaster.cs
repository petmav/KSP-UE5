using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI
{
	[AddComponentMenu("KSP/Graphic Raycaster")]
	[RequireComponent(typeof(Canvas))]
	public class KSPGraphicRaycaster : GraphicRaycaster
	{
		[SerializeField]
		private List<string> inputLockMask = new List<string>();

		private ulong lockMask;

		private bool lockMaskDirty = true;

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
					Debug.Log("KSPGraphicRaycaster '" + base.gameObject.name + "': " + ex.Message);
				}
			}
			lockMaskDirty = false;
		}

		public override void Raycast(PointerEventData eventData, List<RaycastResult> resultAppendList)
		{
			if (lockMaskDirty)
			{
				UpdateLockMask();
			}
			if (lockMask == 0L || InputLockManager.IsUnlocked((ControlTypes)lockMask))
			{
				base.Raycast(eventData, resultAppendList);
			}
		}
	}
}
