using UnityEngine;

public interface IUIContainer : IUIObject, IEZDragDrop
{
	void AddChild(GameObject go);

	void RemoveChild(GameObject go);

	void AddSubject(GameObject go);

	void RemoveSubject(GameObject go);
}
