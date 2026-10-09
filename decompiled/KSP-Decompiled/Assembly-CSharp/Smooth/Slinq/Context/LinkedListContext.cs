using System.Collections.Generic;
using Smooth.Algebraics;

namespace Smooth.Slinq.Context
{
	public struct LinkedListContext<T>
	{
		private bool needsMove;

		private LinkedListNode<T> node;

		private readonly int step;

		private BacktrackDetector bd;

		private static readonly Mutator<T, LinkedListContext<T>> skip = Skip;

		private static readonly Mutator<T, LinkedListContext<T>> remove = Remove;

		private static readonly Mutator<T, LinkedListContext<T>> dispose = Dispose;

		private static readonly Mutator<LinkedListNode<T>, LinkedListContext<T>> skipNodes = SkipNodes;

		private static readonly Mutator<LinkedListNode<T>, LinkedListContext<T>> removeNodes = RemoveNodes;

		private static readonly Mutator<LinkedListNode<T>, LinkedListContext<T>> disposeNodes = DisposeNodes;

		private LinkedListContext(LinkedListNode<T> node, int step)
		{
			needsMove = false;
			this.node = node;
			this.step = step;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<T, LinkedListContext<T>> Slinq(LinkedListNode<T> node, int step)
		{
			return new Slinq<T, LinkedListContext<T>>(skip, remove, dispose, new LinkedListContext<T>(node, step));
		}

		public static Slinq<LinkedListNode<T>, LinkedListContext<T>> SlinqNodes(LinkedListNode<T> node, int step)
		{
			return new Slinq<LinkedListNode<T>, LinkedListContext<T>>(skipNodes, removeNodes, disposeNodes, new LinkedListContext<T>(node, step));
		}

		private static void Skip(ref LinkedListContext<T> context, out Option<T> next)
		{
			if (context.needsMove)
			{
				int i = context.step;
				while (i > 0 && context.node != null)
				{
					context.node = context.node.Next;
					i--;
				}
				for (; i < 0; i++)
				{
					if (context.node == null)
					{
						break;
					}
					context.node = context.node.Previous;
				}
			}
			else
			{
				context.needsMove = true;
			}
			if (context.node == null)
			{
				next = default(Option<T>);
			}
			else
			{
				next = new Option<T>(context.node.Value);
			}
		}

		private static void Remove(ref LinkedListContext<T> context, out Option<T> next)
		{
			if (context.step == 0)
			{
				next = default(Option<T>);
				context.node.List.Remove(context.node);
			}
			else
			{
				LinkedListNode<T> linkedListNode = context.node;
				Skip(ref context, out next);
				linkedListNode.List.Remove(linkedListNode);
			}
		}

		private static void Dispose(ref LinkedListContext<T> context, out Option<T> next)
		{
			next = default(Option<T>);
		}

		private static void SkipNodes(ref LinkedListContext<T> context, out Option<LinkedListNode<T>> next)
		{
			if (context.needsMove)
			{
				int i = context.step;
				while (i > 0 && context.node != null)
				{
					context.node = context.node.Next;
					i--;
				}
				for (; i < 0; i++)
				{
					if (context.node == null)
					{
						break;
					}
					context.node = context.node.Previous;
				}
			}
			else
			{
				context.needsMove = true;
			}
			if (context.node == null)
			{
				next = default(Option<LinkedListNode<T>>);
			}
			else
			{
				next = new Option<LinkedListNode<T>>(context.node);
			}
		}

		private static void RemoveNodes(ref LinkedListContext<T> context, out Option<LinkedListNode<T>> next)
		{
			if (context.step == 0)
			{
				next = default(Option<LinkedListNode<T>>);
				context.node.List.Remove(context.node);
			}
			else
			{
				LinkedListNode<T> linkedListNode = context.node;
				SkipNodes(ref context, out next);
				linkedListNode.List.Remove(linkedListNode);
			}
		}

		private static void DisposeNodes(ref LinkedListContext<T> context, out Option<LinkedListNode<T>> next)
		{
			next = default(Option<LinkedListNode<T>>);
		}
	}
}
