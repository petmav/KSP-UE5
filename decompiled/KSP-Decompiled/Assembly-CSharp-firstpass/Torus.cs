using System.Collections.Generic;
using UnityEngine;

public class Torus : MonoBehaviour
{
	private Rigidbody _controller;

	private List<Collider> _colliders;

	private Vector3 _startPos;

	public void Awake()
	{
		_controller = GameObject.FindGameObjectWithTag("Torus controller").GetComponent<Rigidbody>();
		_colliders = new List<Collider>(GetComponentsInChildren<Collider>());
		_startPos = base.transform.position;
	}

	public void OnCollisionEnter(Collision collision)
	{
		GetComponent<AudioSource>().Play();
		Vector3 position = (collision.collider.CompareTag("Floor") ? _startPos : collision.collider.transform.position);
		GetComponent<Rigidbody>().isKinematic = true;
		_colliders.ForEach(delegate(Collider c)
		{
			c.isTrigger = true;
		});
		base.transform.position = position;
		GetComponent<Rigidbody>().isKinematic = false;
		_colliders.ForEach(delegate(Collider c)
		{
			c.isTrigger = false;
		});
		Vector3 position2 = _controller.transform.position;
		_controller.transform.position = position;
		Vector3 translation = _controller.transform.position - position2;
		Camera.main.transform.Translate(translation, Space.World);
	}
}
