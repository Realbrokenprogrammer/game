om_internal b32
Overlaps(vector2 A, vector2 B)
{
	return !(A.x > B.y || B.x > A.y);
}

om_internal vector2 *
GetPoints(entity_physics_blueprint Shape, u32 *Size)
{
	switch (Shape.CollisionShape)
	{
		case CollisionShape_Circle:
		{
			return (NULL);
		} break;
		case CollisionShape_Triangle:
		{
			vector2 *Axes = (vector2 *)malloc(3 * sizeof(vector2));
			if (Axes == NULL)
			{
				return (NULL);
			}

			*Size = 3;
			Axes[0] = Shape.Triangle.p1;
			Axes[1] = Shape.Triangle.p2;
			Axes[2] = Shape.Triangle.p3;
			
			return (Axes);
		} break;
		case CollisionShape_Rectangle:
		{
			vector2 *Axes = (vector2 *)malloc(4 * sizeof(vector2));
			if (Axes == NULL)
			{
				return (NULL);
			}

			*Size = 4;
			Axes[0] = Shape.Rectangle.Min;
			Axes[1] = { Shape.Rectangle.Max.x, Shape.Rectangle.Min.y };
			Axes[2] = Shape.Rectangle.Max;
			Axes[3] = { Shape.Rectangle.Min.x, Shape.Rectangle.Max.y };
			return (Axes);
		} break;
	}
}

om_internal vector2 *
GetEdgeNormals(vector2 *Vertices, u32 VerticeCount)
{
	u32 EdgeIndex = 0;
	vector2 *EdgeNormals = (vector2 *)malloc(VerticeCount * sizeof(vector2));
	if (EdgeNormals == NULL)
	{
		return (NULL);
	}

	for (u32 VertexIndex = 0; VertexIndex < VerticeCount; ++VertexIndex)
	{
		vector2 P1 = Vertices[VertexIndex];
		vector2 P2;
		if (VertexIndex + 1 >= VerticeCount)
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

		EdgeNormals[EdgeIndex++] = Normal;
	}

	return EdgeNormals;
}

om_internal vector2 *
GetEdges(vector2 *Vertices, u32 VerticeCount)
{
	u32 EdgeIndex = 0;
	vector2 *Edges = (vector2 *)malloc(VerticeCount * sizeof(vector2));
	if (Edges == NULL)
	{
		return (NULL);
	}

	for (u32 VertexIndex = 0; VertexIndex < VerticeCount; ++VertexIndex)
	{
		vector2 P1 = Vertices[VertexIndex];
		vector2 P2;
		if (VertexIndex + 1 >= VerticeCount)
		{
			P2 = Vertices[0];
		}
		else
		{
			P2 = Vertices[VertexIndex + 1];
		}

		vector2 Edge = P2 - P1;

		Edges[EdgeIndex++] = Edge;
	}

	return Edges;
}

om_internal vector2
ProjectOntoAxis(vector2 *Vertices, u32 VerticeCount, vector2 Axis)
{
	vector2 Projection = {};
	Projection.x = Inner(Axis, Vertices[0]);
	Projection.y = Projection.x;

	for (u32 VertexIndex = 1; VertexIndex < VerticeCount; ++VertexIndex)
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

enum VoronoiRegion
{
	VoronoiRegion_Left,
	VoronoiRegion_Middle,
	VoronoiRegion_Right
};

om_internal VoronoiRegion
GetVoronoiRegion(vector2 Line, vector2 Point)
{
	r32 LineLengthSquared = LengthSquared(Line);
	r32 PointDotLine = Inner(Point, Line);

	// If the point is beyond the start of the line then it is in the left region.
	// Else if its beyond the end of the line then it is in the right region.
	// Otherwise its in the middle.
	if (PointDotLine < 0)
	{
		return (VoronoiRegion_Left);
	}
	else if (PointDotLine > LineLengthSquared)
	{
		return (VoronoiRegion_Right);
	}
	else
	{
		return (VoronoiRegion_Middle);
	}
}

om_internal collision_info
TestCircles(circle CircleA, circle CircleB)
{
	collision_info Result = {};
	Result.IsColliding = false;

	// Check if the distance between the center of the two circles
	// are greater than their combined radius.
	vector2 Distance = CircleB.Center - CircleA.Center;
	r32 Radius = CircleA.Radius + CircleB.Radius;
	r32 Magnitude = LengthSquared(Distance);

	if (Magnitude < (Radius * Radius)) 
	{
		Result.IsColliding = true;
		Result.PenetrationDepth = (Radius - Length(Distance));
		Result.PenetrationNormal = Normalize(Distance);
	}

	return (Result);
}

om_internal collision_info
TestPolygonCircle(entity_physics_blueprint Shape, circle Circle)
{
	collision_info Result = {};
	Result.IsColliding = true;

	vector2 CirclePosition = Circle.Center;
	r32 RadiusSquared = Circle.Radius * Circle.Radius;

	u32 VerticesSize = 0;
	vector2 *Vertices = GetPoints(Shape, &VerticesSize);

	vector2 *Axes = GetEdges(Vertices, VerticesSize);

	for (u32 VerticeIndex = 0; VerticeIndex < VerticesSize; ++VerticeIndex)
	{
		r32 Overlap = 0.0f;
		vector2 OverlapNormal = {};

		// Center of circle relative to the current vertex.
		vector2 Point = CirclePosition - Vertices[VerticeIndex];
		vector2 Axis = Axes[VerticeIndex];

		// Determine Voronoi region the center of the circle is within.
		VoronoiRegion Region = GetVoronoiRegion(Axis, Point);
		if (Region == VoronoiRegion_Left)
		{
			u32 PreviousIndex;
			if (VerticeIndex == 0)
			{
				PreviousIndex = VerticesSize - 1;
			}
			else
			{
				PreviousIndex = VerticeIndex - 1;
			}

			// Make sure we're in the Right Voronoi region on the previous edge.
			Axis = Axes[PreviousIndex];
			vector2 Point2 = CirclePosition - Vertices[PreviousIndex];
			Region = GetVoronoiRegion(Axis, Point2);
			if (Region == VoronoiRegion_Right)
			{
				// Check if the circle intersects the point.
				r32 Distance = Length(Point);
				if (Distance > Circle.Radius)
				{
					// No intersection
					Result.IsColliding = false;
					
					free(Axes);
					free(Vertices);
					return (Result);
				}
				else
				{
					// It intersects
					Overlap = Circle.Radius - Distance;
					OverlapNormal = Normalize(Point);
				}
			}
		}
		else if (Region == VoronoiRegion_Right)
		{
			u32 NextIndex;
			if (VerticeIndex == VerticesSize - 1)
			{
				NextIndex = 0;
			}
			else
			{
				NextIndex = VerticeIndex + 1;
			}

			// Make sure we're in the Left Voronoi region on the previous edge.
			Axis = Axes[NextIndex];
			vector2 Point = CirclePosition - Vertices[NextIndex];
			Region = GetVoronoiRegion(Axis, Point);
			if (Region == VoronoiRegion_Left)
			{
				// Check if the circle intersects the point.
				r32 Distance = Length(Point);
				if (Distance > Circle.Radius)
				{
					// No intersection
					Result.IsColliding = false;
					
					free(Axes);
					free(Vertices);
					return (Result);
				}
				else
				{
					// It intersects
					Overlap = Circle.Radius - Distance;
					OverlapNormal = Normalize(Point);
				}
			}
		}
		else
		{
			// Check if the circle is intersecting the edge.
			// First transforming the edge into it's edge normal then retrieving the
			// perpendicular distance between the center of the circle and the edge.
			vector2 Normal = Perp(Axis);
			Normal = Normalize(Normal);
			r32 Distance = Inner(Point, Normal);
			r32 DistanceAbsoluteValue = AbsoluteValue(Distance);

			// If the circle is on the outside of the edge.
			if (Distance > 0 && DistanceAbsoluteValue > Circle.Radius)
			{
				// No intersection
				Result.IsColliding = false;
				
				free(Axes);
				free(Vertices);
				return (Result);
			}
			else
			{
				// It intersects
				Overlap = Circle.Radius - Distance;
				OverlapNormal = Normal;
			}
		}

		//TODO: Look at this again.
		// If this is the new smallest overlap found then store it.
		if (AbsoluteValue(Overlap) < AbsoluteValue(Result.PenetrationDepth))
		{
			Result.PenetrationDepth = Overlap;
			Result.PenetrationNormal = OverlapNormal;
		}
	}

	free(Axes);
	free(Vertices);
	return (Result);
}

om_internal collision_info
TestCollision(entity_physics_blueprint ShapeA, entity_physics_blueprint ShapeB)
{
	collision_info Result = {};
	Result.IsColliding = true;

	if (ShapeA.CollisionShape == CollisionShape_Circle && ShapeB.CollisionShape == CollisionShape_Circle)
	{
		return TestCircles(ShapeA.Circle, ShapeB.Circle);
	}

	if (ShapeA.CollisionShape == CollisionShape_Circle)
	{
		return TestPolygonCircle(ShapeB, ShapeA.Circle);
	}
	if (ShapeB.CollisionShape == CollisionShape_Circle)
	{
		return TestPolygonCircle(ShapeA, ShapeB.Circle);
	}

	// Both Shapes are polygons
	u32 Vertices1Size = 0;
	vector2 *Vertices1 = GetPoints(ShapeA, &Vertices1Size);
	
	u32 Vertices2Size = 0;
	vector2 *Vertices2 = GetPoints(ShapeB, &Vertices2Size);

	vector2 *Axes1 = GetEdgeNormals(Vertices1, Vertices1Size);
	vector2 *Axes2 = GetEdgeNormals(Vertices2, Vertices2Size);

	r32 Overlap = R32MAX;
	vector2 MinPenetrationAxis = {};

	// Loop over all separating axes in first shape's axes. If we find one that 
	// doesn't overlap then there's no collision.
	for (u32 AxesIndex = 0; AxesIndex < Vertices1Size; ++AxesIndex)
	{
		vector2 Axis = Axes1[AxesIndex];
	
		// Project both shapes onto the current axis
		vector2 Projection1 = ProjectOntoAxis(Vertices1, Vertices1Size, Axis);
		vector2 Projection2 = ProjectOntoAxis(Vertices2, Vertices2Size, Axis);

		// Check for overlap
		if (!Overlaps(Projection1, Projection2))
		{
			Result.IsColliding = false;
			
			free(Axes1);
			free(Axes2);
			free(Vertices1);
			free(Vertices2);
			return (Result);
		}

		// Get the amount of overlap
		r32 O = OM_MIN(Projection1.y, Projection2.y) - OM_MAX(Projection1.x, Projection2.x);
		if (O < Overlap)
		{
			Overlap = O;
			MinPenetrationAxis = Axis;
		}
	}

	// Loop over all separating axes in second shape's axes. If we find one that 
	// doesn't overlap then there's no collision.
	for (u32 AxesIndex = 0; AxesIndex < Vertices2Size; ++AxesIndex)
	{
		vector2 Axis = Axes2[AxesIndex];

		// Project both shapes onto the current axis
		vector2 Projection1 = ProjectOntoAxis(Vertices1, Vertices1Size, Axis);
		vector2 Projection2 = ProjectOntoAxis(Vertices2, Vertices2Size, Axis);

		// Check for overlap
		if (!Overlaps(Projection1, Projection2))
		{
			Result.IsColliding = false;
			
			free(Axes1);
			free(Axes2);
			free(Vertices1);
			free(Vertices2);
			return (Result);
		}

		// Get the amount of overlap
		r32 O = OM_MIN(Projection1.y, Projection2.y) - OM_MAX(Projection1.x, Projection2.x);
		if (O < Overlap)
		{
			Overlap = O;
			MinPenetrationAxis = Axis;
		}
	}
	
	Result.PenetrationDepth = Overlap;
	Result.PenetrationNormal = MinPenetrationAxis;

	free(Axes1);
	free(Axes2);
	free(Vertices1);
	free(Vertices2);
	return (Result);
}