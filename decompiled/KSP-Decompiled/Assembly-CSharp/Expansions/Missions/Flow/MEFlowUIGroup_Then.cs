using System;
using UnityEngine;
using UnityEngine.UI;

namespace Expansions.Missions.Flow
{
	public class MEFlowUIGroup_Then : MonoBehaviour
	{
		[SerializeField]
		private Image bracket;

		[SerializeField]
		internal Transform ChildHolder;

		private MEFlowThenBlock thenBlock;

		private MEFlowParser parser;

		internal static UnityEngine.Object groupPrefab;

		private Color bracketColor;

		public static MEFlowUIGroup_Then Create(MEFlowThenBlock thenBlock, MEFlowParser parser)
		{
			GameObject obj = (GameObject)UnityEngine.Object.Instantiate(groupPrefab);
			obj.transform.localPosition = Vector3.zero;
			MEFlowUIGroup_Then component = obj.GetComponent<MEFlowUIGroup_Then>();
			component.thenBlock = thenBlock;
			thenBlock.OnUpdateFlowUI = (Callback<MEFlowParser>)Delegate.Combine(thenBlock.OnUpdateFlowUI, new Callback<MEFlowParser>(component.OnUpdateFlowUI));
			component.bracketColor = component.bracket.color;
			component.parser = parser;
			return component;
		}

		private void OnDestroy()
		{
			MEFlowThenBlock mEFlowThenBlock = thenBlock;
			mEFlowThenBlock.OnUpdateFlowUI = (Callback<MEFlowParser>)Delegate.Remove(mEFlowThenBlock.OnUpdateFlowUI, new Callback<MEFlowParser>(OnUpdateFlowUI));
		}

		private void OnUpdateFlowUI(MEFlowParser parser)
		{
			bool num = parser.showNonObjectives || thenBlock.HasObjectives;
			bool flag = !parser.showEvents && !thenBlock.HasVisibleChildren;
			bool flag2 = num && !flag;
			if (base.gameObject.activeSelf != flag2)
			{
				base.gameObject.SetActive(flag2);
			}
			if (num && !flag)
			{
				SetUnreachable(!thenBlock.HasReachableObjectives);
			}
		}

		public void SetBracketColor(Color color)
		{
			bracketColor = color;
			bracket.color = color;
		}

		public void SetUnreachable(bool unreachable = true)
		{
			bracket.color = new Color(bracketColor.r, bracketColor.g, bracketColor.b, bracketColor.a / (float)((!unreachable) ? 1 : 2));
		}
	}
}
