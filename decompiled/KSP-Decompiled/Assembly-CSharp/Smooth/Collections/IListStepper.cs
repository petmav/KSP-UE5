using System.Collections;
using System.Collections.Generic;

namespace Smooth.Collections
{
	public class IListStepper<T> : IEnumerable<T>, IEnumerable
	{
		private readonly IList<T> list;

		private readonly int startIndex;

		private readonly int step;

		private IListStepper()
		{
		}

		public IListStepper(IList<T> list, int startIndex, int step)
		{
			this.list = list;
			this.startIndex = startIndex;
			this.step = step;
		}

		public IEnumerator<T> GetEnumerator()
		{
			for (int i = startIndex; 0 <= i && i < list.Count; i += step)
			{
				yield return list[i];
			}
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return GetEnumerator();
		}
	}
}
