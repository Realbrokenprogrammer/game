#include <vector>

struct shape 
{
	collision_shape CollisionShape;
	union
	{
		line Line;
		rect2 Rectangle;
		circle Circle;
		triangle Triangle;
	};
};

vector2 TripleProduct(vector2 a, vector2 b, vector2 c) 
{
	vector3 A = { a.x, a.y, 0.0f };
	vector3 B = { b.x, b.y, 0.0f };
	vector3 C = { c.x, c.y, 0.0f };

	vector3 f = Cross(A, B);
	vector3 s = Cross(f, C);
	
	vector2 Result = { s.x, s.y };

	return (Result);
}

om_internal b32
Overlaps(vector2 A, vector2 B)
{
	return !(A.x > B.y || B.x > A.y);
}

om_internal b32
TestCircles(circle CircleA, circle CircleB)
{
	r32 x = CircleA.Center.x - CircleB.Center.x;
	r32 y = CircleA.Center.y - CircleB.Center.y;
	
	r32 CenterDistanceSq = x * x + y * y;
	
	r32 Radius = CircleA.Radius + CircleB.Radius;
	r32 RadiusSq = Radius * Radius;

	return (CenterDistanceSq <= RadiusSq);
}

om_internal std::vector<vector2>
GetPoints(shape Shape)
{
	switch (Shape.CollisionShape)
	{
		case CollisionShape_Circle:
		{
			//return (NULL);
		} break;
		case CollisionShape_Triangle:
		{
			std::vector<vector2> Axes(3);
			Axes[0] = Shape.Triangle.p1;
			Axes[1] = Shape.Triangle.p2;
			Axes[2] = Shape.Triangle.p3;
			return (Axes);
		} break;
		case CollisionShape_Rectangle:
		{
			std::vector<vector2> Axes(4);
			Axes[0] = Shape.Rectangle.Min;
			Axes[1] = { Shape.Rectangle.Max.x, Shape.Rectangle.Min.y };
			Axes[2] = Shape.Rectangle.Max;
			Axes[3] = { Shape.Rectangle.Min.x, Shape.Rectangle.Max.y };
			return (Axes);
		} break;
	}
}

om_internal std::vector<vector2>
GetEdgeNormals(std::vector<vector2> Vertices)
{
	std::vector<vector2> EdgeNormals;

	for (u32 VertexIndex = 0; VertexIndex < Vertices.size(); ++VertexIndex)
	{
		vector2 P1 = Vertices[VertexIndex];
		vector2 P2;
		if (VertexIndex + 1 >= Vertices.size())
		{
			P2 = Vertices[0];
		}
		else
		{
			P2 = Vertices[VertexIndex + 1];
		}

		vector2 Edge = P1 - P2;
		vector2 Normal = Perp(Edge);
		Normal = Normalize(Normal);

		EdgeNormals.push_back(Normal);
	}

	return EdgeNormals;
}

om_internal vector2
ProjectAxis(std::vector<vector2> Vertices, vector2 Axis)
{
	vector2 Projection = {};
	Projection.x = Inner(Axis, Vertices[0]);
	Projection.y = Projection.x;

	for (u32 VertexIndex = 1; VertexIndex < Vertices.size(); ++VertexIndex)
	{
		r32 P = Inner(Axis, Vertices[VertexIndex]);

		if (P < Projection.x)
		{
			Projection.x = P;
		}
		else if (P > Projection.y)
		{
			Projection.y = P; 
		}
	}

	return (Projection);
}

//TODO: Test for Poly, Circle
//TODO: Replace usage of std::vector
//TODO: Add propper return value (New struct containing collision info)
//TODO: Clean up Drawing code in Game.cpp
//TODO: Clean up collision testing code in Game.cpp
//TODO: Improve structure and naming of functions
//TODO: Implement this collision check in MoveEntity
om_internal b32
Test(shape ShapeA, shape ShapeB)
{
	if (ShapeA.CollisionShape == CollisionShape_Circle && ShapeB.CollisionShape == CollisionShape_Circle)
	{
		return TestCircles(ShapeA.Circle, ShapeB.Circle);
	}

	// Both Shapes are polygons
	std::vector<vector2> Vertices1 = GetPoints(ShapeA);
	std::vector<vector2> Vertices2 = GetPoints(ShapeB);

	std::vector<vector2> Axes1 = GetEdgeNormals(Vertices1);
	std::vector<vector2> Axes2 = GetEdgeNormals(Vertices2);

	r32 Overlap = R32MAX;
	vector2 MinPenetrationAxis;

	for (u32 AxesIndex = 0; AxesIndex < Axes1.size(); ++AxesIndex)
	{
		vector2 Axis = Axes1[AxesIndex];
	
		// Project both shapes onto the current axis
		vector2 Projection1 = ProjectAxis(Vertices1, Axis);
		vector2 Projection2 = ProjectAxis(Vertices2, Axis);

		// Check for overlap
		if (!Overlaps(Projection1, Projection2))
		{
			return (false);
		}

		// Get the amount of overlap
		r32 O = OM_MIN(Projection1.y, Projection2.y) - OM_MAX(Projection1.x, Projection2.x);
		if (O < Overlap)
		{
			Overlap = O;
			MinPenetrationAxis = Axis;
		}
	}

	for (u32 AxesIndex = 0; AxesIndex < Axes2.size(); ++AxesIndex)
	{
		vector2 Axis = Axes2[AxesIndex];

		// Project both shapes onto the current axis
		vector2 Projection1 = ProjectAxis(Vertices1, Axis);
		vector2 Projection2 = ProjectAxis(Vertices2, Axis);

		// Check for overlap
		if (!Overlaps(Projection1, Projection2))
		{
			return (false);
		}

		// Get the amount of overlap
		r32 O = OM_MIN(Projection1.y, Projection2.y) - OM_MAX(Projection1.x, Projection2.x);
		if (O < Overlap)
		{
			Overlap = O;
			MinPenetrationAxis = Axis;
		}
	}
	 

	//TODO: Return theese for collision handling.
	// Overlap = Penetration depth
	// MinPenetrationAxis = Penetration normal

	return (true);
}