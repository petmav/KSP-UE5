using System.Collections.Generic;
using UnityEngine.UI;

namespace UnityEngine.EventSystems;

[AddComponentMenu("Event/Physics 2D Raycaster")]
[RequireComponent(typeof(Camera))]
public class Physics2DRaycaster : PhysicsRaycaster
{
	private RaycastHit2D[] m_Hits;

	protected Physics2DRaycaster()
	{
	}

	public override void Raycast(PointerEventData eventData, List<RaycastResult> resultAppendList)
	{
		Ray ray = default(Ray);
		float distanceToClipPlane = 0f;
		int eventDisplayIndex = 0;
		if (!ComputeRayAndDistance(eventData, ref ray, ref eventDisplayIndex, ref distanceToClipPlane))
		{
			return;
		}
		int num = 0;
		if (base.maxRayIntersections == 0)
		{
			if (ReflectionMethodsCache.Singleton.getRayIntersectionAll == null)
			{
				return;
			}
			m_Hits = ReflectionMethodsCache.Singleton.getRayIntersectionAll(ray, distanceToClipPlane, base.finalEventMask);
			num = m_Hits.Length;
		}
		else
		{
			if (ReflectionMethodsCache.Singleton.getRayIntersectionAllNonAlloc == null)
			{
				return;
			}
			if (m_LastMaxRayIntersections != m_MaxRayIntersections)
			{
				m_Hits = new RaycastHit2D[base.maxRayIntersections];
				m_LastMaxRayIntersections = m_MaxRayIntersections;
			}
			num = ReflectionMethodsCache.Singleton.getRayIntersectionAllNonAlloc(ray, m_Hits, distanceToClipPlane, base.finalEventMask);
		}
		if (num != 0)
		{
			int i = 0;
			for (int num2 = num; i < num2; i++)
			{
				SpriteRenderer component = m_Hits[i].collider.gameObject.GetComponent<SpriteRenderer>();
				RaycastResult raycastResult = default(RaycastResult);
				raycastResult.gameObject = m_Hits[i].collider.gameObject;
				raycastResult.module = this;
				raycastResult.distance = Vector3.Distance(eventCamera.transform.position, m_Hits[i].point);
				raycastResult.worldPosition = m_Hits[i].point;
				raycastResult.worldNormal = m_Hits[i].normal;
				raycastResult.screenPosition = eventData.position;
				raycastResult.displayIndex = eventDisplayIndex;
				raycastResult.index = resultAppendList.Count;
				raycastResult.sortingLayer = ((component != null) ? component.sortingLayerID : 0);
				raycastResult.sortingOrder = ((component != null) ? component.sortingOrder : 0);
				RaycastResult item = raycastResult;
				resultAppendList.Add(item);
			}
		}
	}
}
