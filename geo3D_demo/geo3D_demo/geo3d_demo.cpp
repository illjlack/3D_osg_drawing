/*
 * A simple OpenSceneGraph demonstration that defines a custom Geo3D node
 * holding several specialized geometry types.  Each geometry class derives
 * from osg::Geometry and uses the META_Node macro to provide runtime
 * type information, cloning and serialization support.  A custom Geo3D
 * class derives from osg::Group so it can contain multiple child nodes.
 *
 * The scene created in main() instantiates one object of each geometry
 * class, populates it with a few vertices/edges/faces, wraps it in an
 * osg::Geode and adds that to a Geo3D node.  A custom visitor
 * (Geo3DVisitor) traverses the scene graph and detects our custom
 * geometry types.  Finally the demo writes the Geo3D node to an .osg
 * file and reads it back to demonstrate I/O support provided by
 * META_Node.
 *
 * This example is intended as a starting point for applications that
 * need to attach domain‑specific geometry objects to an OpenSceneGraph
 * scene graph, traverse them with the visitor pattern and save/load
 * them through OSG’s native file formats.  It was inspired by the
 * "AnimatingSwitch" NodeKit example in the OSG 3.0 Beginner’s Guide,
 * which shows how to derive new node types and register them with
 * the META_Node macro【288025376384834†L603-L618】.
 */

#include <osg/Group>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Vec3>
#include <osg/Array>
#include <osg/DrawArrays>
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <iostream>

// Define a point‑cloud geometry.  Each vertex will be drawn as an
// individual point.  The META_Node macro supplies run‑time type
// information and registers the class with the OSG serialization system.
class VertexGeometry : public osg::Geometry
{
public:
    VertexGeometry() : osg::Geometry() {}
    VertexGeometry(const VertexGeometry& copy, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
      : osg::Geometry(copy, copyop) {}
    META_Node(osg, VertexGeometry);
};

// Define a polyline geometry.  Vertices will be drawn as individual
// line segments (GL_LINES).  Use META_Node for serialization and
// cloning support.
class EdgeGeometry : public osg::Geometry
{
public:
    EdgeGeometry() : osg::Geometry() {}
    EdgeGeometry(const EdgeGeometry& copy, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
      : osg::Geometry(copy, copyop) {}
    META_Node(osg, EdgeGeometry);
};

// Define a surface geometry.  Vertices will be drawn as triangles
// (GL_TRIANGLES).  META_Node again provides the OSG plumbing.
class FaceGeometry : public osg::Geometry
{
public:
    FaceGeometry() : osg::Geometry() {}
    FaceGeometry(const FaceGeometry& copy, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
      : osg::Geometry(copy, copyop) {}
    META_Node(osg, FaceGeometry);
};

// Define a control point geometry.  For demonstration purposes this
// class behaves like VertexGeometry but is separated by type so that
// visitors can treat it differently.
class ControlPointGeometry : public osg::Geometry
{
public:
    ControlPointGeometry() : osg::Geometry() {}
    ControlPointGeometry(const ControlPointGeometry& copy, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
      : osg::Geometry(copy, copyop) {}
    META_Node(osg, ControlPointGeometry);
};

// Define a bounding box geometry.  It draws the edges of a box using
// line loops.  The bounding box could be computed from other
// geometry, but here we just specify its corners manually.
class BoundingBoxGeometry : public osg::Geometry
{
public:
    BoundingBoxGeometry() : osg::Geometry() {}
    BoundingBoxGeometry(const BoundingBoxGeometry& copy, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
      : osg::Geometry(copy, copyop) {}
    META_Node(osg, BoundingBoxGeometry);
};

// Define a Geo3D node that can contain our custom geometry.  It
// derives from osg::Group so it can hold child nodes.  The
// META_Node macro registers it with the OSG type system and ensures
// instances can be cloned and serialized【288025376384834†L603-L618】.
class Geo3D : public osg::Group
{
public:
    Geo3D() : osg::Group() {}
    Geo3D(const Geo3D& copy, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
      : osg::Group(copy, copyop) {}
    META_Node(osg, Geo3D);
};

// Helper to create a simple point cloud.  It fills a VertexGeometry
// instance with a few vertices and configures it to draw GL_POINTS.
static osg::ref_ptr<VertexGeometry> createVertexGeometry()
{
    osg::ref_ptr<VertexGeometry> geom = new VertexGeometry;
    osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
    // Add some arbitrary points
    verts->push_back(osg::Vec3(-1.0f, 0.0f, 0.0f));
    verts->push_back(osg::Vec3(0.0f, 1.0f, 0.0f));
    verts->push_back(osg::Vec3(1.0f, 0.0f, 0.0f));
    geom->setVertexArray(verts.get());
    // Use DrawArrays to specify primitive type and count
    geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, verts->size()));
    return geom;
}

// Helper to create a simple polyline consisting of two connected lines.
static osg::ref_ptr<EdgeGeometry> createEdgeGeometry()
{
    osg::ref_ptr<EdgeGeometry> geom = new EdgeGeometry;
    osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
    // Define four vertices; pairs will form two line segments
    verts->push_back(osg::Vec3(-1.0f, -1.0f, 0.0f));
    verts->push_back(osg::Vec3(0.0f,  0.0f, 0.0f));
    verts->push_back(osg::Vec3(0.0f,  0.0f, 0.0f));
    verts->push_back(osg::Vec3(1.0f, -1.0f, 0.0f));
    geom->setVertexArray(verts.get());
    geom->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, verts->size()));
    return geom;
}

// Helper to create a simple triangle fan (three vertices forming one
// triangle).  FaceGeometry will be drawn using GL_TRIANGLES.
static osg::ref_ptr<FaceGeometry> createFaceGeometry()
{
    osg::ref_ptr<FaceGeometry> geom = new FaceGeometry;
    osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
    // Define a single triangle in the XZ plane
    verts->push_back(osg::Vec3(-1.0f, 0.0f, -1.0f));
    verts->push_back(osg::Vec3( 1.0f, 0.0f, -1.0f));
    verts->push_back(osg::Vec3( 0.0f, 0.0f,  1.0f));
    geom->setVertexArray(verts.get());
    geom->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, verts->size()));
    return geom;
}

// Helper to create control point geometry.  For demonstration it
// behaves like a point cloud but could be extended with
// application‑specific attributes.
static osg::ref_ptr<ControlPointGeometry> createControlPointGeometry()
{
    osg::ref_ptr<ControlPointGeometry> geom = new ControlPointGeometry;
    osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
    verts->push_back(osg::Vec3(0.0f, 0.0f, 1.0f));
    verts->push_back(osg::Vec3(0.5f, 0.5f, 1.5f));
    geom->setVertexArray(verts.get());
    geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, verts->size()));
    return geom;
}

// Helper to create a bounding box geometry given min/max extents.  It
// draws the twelve edges of the box using line strips.
static osg::ref_ptr<BoundingBoxGeometry> createBoundingBoxGeometry(const osg::Vec3& minCorner, const osg::Vec3& maxCorner)
{
    osg::ref_ptr<BoundingBoxGeometry> geom = new BoundingBoxGeometry;
    osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
    // Define 8 corners of the box
    osg::Vec3 v0(minCorner.x(), minCorner.y(), minCorner.z());
    osg::Vec3 v1(maxCorner.x(), minCorner.y(), minCorner.z());
    osg::Vec3 v2(maxCorner.x(), maxCorner.y(), minCorner.z());
    osg::Vec3 v3(minCorner.x(), maxCorner.y(), minCorner.z());
    osg::Vec3 v4(minCorner.x(), minCorner.y(), maxCorner.z());
    osg::Vec3 v5(maxCorner.x(), minCorner.y(), maxCorner.z());
    osg::Vec3 v6(maxCorner.x(), maxCorner.y(), maxCorner.z());
    osg::Vec3 v7(minCorner.x(), maxCorner.y(), maxCorner.z());
    // Add line segments for the 12 edges of a box
    // Bottom rectangle
    verts->push_back(v0); verts->push_back(v1);
    verts->push_back(v1); verts->push_back(v2);
    verts->push_back(v2); verts->push_back(v3);
    verts->push_back(v3); verts->push_back(v0);
    // Top rectangle
    verts->push_back(v4); verts->push_back(v5);
    verts->push_back(v5); verts->push_back(v6);
    verts->push_back(v6); verts->push_back(v7);
    verts->push_back(v7); verts->push_back(v4);
    // Vertical edges
    verts->push_back(v0); verts->push_back(v4);
    verts->push_back(v1); verts->push_back(v5);
    verts->push_back(v2); verts->push_back(v6);
    verts->push_back(v3); verts->push_back(v7);
    geom->setVertexArray(verts.get());
    geom->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, verts->size()));
    return geom;
}

// Custom visitor that locates our custom geometry classes.  The
// visitor traverses all nodes and, when it encounters a Geode,
// examines each Drawable to determine its type.  This demonstrates
// how to process application‑specific Drawables without modifying
// their interface, following the visitor pattern described in the
// documentation【288025376384834†L689-L706】.
class Geo3DVisitor : public osg::NodeVisitor
{
public:
    Geo3DVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
    virtual void apply(osg::Geode& geode)
    {
        for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
        {
            osg::Drawable* drawable = geode.getDrawable(i);
            if (dynamic_cast<VertexGeometry*>(drawable))
                std::cout << "Visited VertexGeometry\n";
            else if (dynamic_cast<EdgeGeometry*>(drawable))
                std::cout << "Visited EdgeGeometry\n";
            else if (dynamic_cast<FaceGeometry*>(drawable))
                std::cout << "Visited FaceGeometry\n";
            else if (dynamic_cast<ControlPointGeometry*>(drawable))
                std::cout << "Visited ControlPointGeometry\n";
            else if (dynamic_cast<BoundingBoxGeometry*>(drawable))
                std::cout << "Visited BoundingBoxGeometry\n";
        }
        traverse(geode);
    }
};

int main(int argc, char** argv)
{
    // Create our custom Geo3D node
    osg::ref_ptr<Geo3D> root = new Geo3D;
    // Create and attach the various geometries wrapped in Geodes
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(createVertexGeometry().get());
        root->addChild(geode.get());
    }
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(createEdgeGeometry().get());
        root->addChild(geode.get());
    }
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(createFaceGeometry().get());
        root->addChild(geode.get());
    }
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(createControlPointGeometry().get());
        root->addChild(geode.get());
    }
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(createBoundingBoxGeometry(osg::Vec3(-1.0f,-1.0f,-1.0f), osg::Vec3(1.0f,1.0f,1.0f)).get());
        root->addChild(geode.get());
    }
    
    // Traverse the scene with our custom visitor
    Geo3DVisitor visitor;
    root->accept(visitor);
    
    // Demonstrate writing the scene to a file.  Because all of our
    // classes use META_Node, osgDB::writeNodeFile() can serialize
    // them.  The returned boolean indicates success.
    osgDB::writeNodeFile(*root, "geo3d_output.osg");
    
    // Demonstrate reading the scene back from disk.  osgDB::readNodeFile
    // returns a ref_ptr<osg::Node> which can be safely cast back to
    // Geo3D if desired.  When loaded at runtime the custom classes
    // will be instantiated automatically.
    osg::ref_ptr<osg::Node> loaded = osgDB::readNodeFile("geo3d_output.osg");
    if (!loaded)
    {
        std::cerr << "Failed to load geo3d_output.osg" << std::endl;
        return 1;
    }
    
    // Set up a simple viewer to display the loaded scene.  This part
    // only runs if OpenSceneGraph is properly installed and linked.
    osgViewer::Viewer viewer;
    viewer.setSceneData(loaded.get());
    return viewer.run();
}