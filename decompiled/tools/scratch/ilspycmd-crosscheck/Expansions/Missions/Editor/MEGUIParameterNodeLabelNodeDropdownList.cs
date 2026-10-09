using System;

namespace Expansions.Missions.Editor
{
	[MEGUI_NodeLabelNodeSelect]
	public class MEGUIParameterNodeLabelNodeDropdownList : MEGUIParameterNodeDropdownList
	{
		protected override void Setup(string name)
		{
			ResetToValidNodeWhenNodeIsMissing = true;
			base.Setup(name);
		}

		protected override DictionaryValueList<Guid, MENode> GetNodeList()
		{
			DictionaryValueList<Guid, MENode> dictionaryValueList = new DictionaryValueList<Guid, MENode>();
			for (int i = 0; i < MissionEditorLogic.Instance.EditorMission.nodes.Count; i++)
			{
				MENode mENode = MissionEditorLogic.Instance.EditorMission.nodes.At(i);
				if (mENode.IsNodeLabelNode)
				{
					dictionaryValueList.Add(mENode.id, mENode);
				}
			}
			return dictionaryValueList;
		}
	}
}
