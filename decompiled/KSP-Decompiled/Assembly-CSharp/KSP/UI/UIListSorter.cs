using System;
using UnityEngine;

namespace KSP.UI
{
	public class UIListSorter : MonoBehaviour
	{
		public delegate void OnSort(int mode, bool asc);

		public UIStateImage[] sortingButtonStates;

		public UIStateImage startSortingMode;

		public bool startSortingAsc = true;

		private OnSort onSort = delegate
		{
		};

		private UIStateImage activeSortingMode;

		private bool activeSortingAsc;

		private UIStateImage lastSortingMode;

		private bool[] lastValue;

		public int StartSortingIndex
		{
			get
			{
				int num = 0;
				int num2 = sortingButtonStates.Length;
				while (true)
				{
					if (num < num2)
					{
						if (sortingButtonStates[num] == startSortingMode)
						{
							break;
						}
						num++;
						continue;
					}
					return -1;
				}
				return num;
			}
		}

		private void Start()
		{
			SetSortingMode(startSortingMode, startSortingAsc);
			lastValue = new bool[sortingButtonStates.Length];
			int i = 0;
			for (int num = lastValue.Length; i < num; i++)
			{
				lastValue[i] = startSortingAsc;
			}
		}

		private void SetSortingMode(UIStateImage mode, bool asc)
		{
			lastSortingMode = activeSortingMode;
			activeSortingMode = mode;
			activeSortingAsc = asc;
			int i = 0;
			for (int num = sortingButtonStates.Length; i < num; i++)
			{
				if (sortingButtonStates[i] != activeSortingMode)
				{
					sortingButtonStates[i].SetState(0);
				}
				else
				{
					sortingButtonStates[i].SetState(activeSortingAsc ? 1 : 2);
				}
			}
		}

		public void ClickButton(int button)
		{
			activeSortingMode = sortingButtonStates[button];
			int num = 0;
			int num2 = sortingButtonStates.Length;
			while (num2-- > 0)
			{
				if (sortingButtonStates[num2] == activeSortingMode)
				{
					num = num2;
					break;
				}
			}
			if (lastSortingMode == activeSortingMode)
			{
				activeSortingAsc = !activeSortingAsc;
				lastValue[num] = activeSortingAsc;
			}
			else
			{
				activeSortingAsc = lastValue[num];
			}
			SetSortingMode(activeSortingMode, activeSortingAsc);
			onSort(button, activeSortingAsc);
		}

		public void AddOnSortCallback(OnSort del)
		{
			onSort = (OnSort)Delegate.Combine(onSort, del);
		}

		public void RemoveOnSortCallback(OnSort del)
		{
			onSort = (OnSort)Delegate.Remove(onSort, del);
		}
	}
}
