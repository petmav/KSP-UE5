using System.Collections;
using System.Collections.Generic;

namespace Smooth.Collections
{
	public class LinkedListStepper<T> : IEnumerable<T>, IEnumerable
	{
		private readonly LinkedListNode<T> startNode;

		private readonly int step;

		private LinkedListStepper()
		{
		}

		public LinkedListStepper(LinkedListNode<T> startNode, int step)
		{
			this.startNode = startNode;
			this.step = step;
		}

		public IEnumerator<T> GetEnumerator()
		{
			LinkedListNode<T> node = startNode;
			while (node != null)
			{
				yield return node.Value;
				int i = step;
				while (i > 0 && node != null)
				{
					node = node.Next;
					i--;
				}
				for (; i < 0; i++)
				{
					if (node == null)
					{
						break;
					}
					node = node.Previous;
				}
			}
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return GetEnumerator();
		}
	}
}
