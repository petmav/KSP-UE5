using System.Linq;
using UnityEngine;

namespace Smooth.Slinq.Test
{
	public class DistinctLinq : MonoBehaviour
	{
		private void Update()
		{
			for (int i = 0; i < SlinqTest.loops; i++)
			{
				SlinqTest.tuples1.Distinct(SlinqTest.eq_1).Count();
			}
		}
	}
}
