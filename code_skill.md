## 3D 射线与拾取

```cpp
// 射线结构（用于拾取）
struct Ray3D
{
    glm::vec3 origin;    // 射线起点
    glm::vec3 direction; // 射线方向（单位向量）
    
    // 默认构造
    Ray3D() : origin(0.0f), direction(0.0f, 0.0f, -1.0f) {}
    
    // 带参构造
    Ray3D(const glm::vec3& orig, const glm::vec3& dir)
        : origin(orig), direction(glm::normalize(dir)) {}
    
    // 返回射线在距离 t 处的点：P(t) = origin + t * direction
    glm::vec3 pointAt(float t) const { return origin + t * direction; }
};

// 拾取结果
struct PickResult3D
{
    bool        hit;       // 是否命中
    float       distance;  // 从射线原点到交点的距离
    glm::vec3   point;     // 交点坐标
    glm::vec3   normal;    // 交点处法线
    void*       userData;  // 几何对象指针
    
    // 默认构造
    PickResult3D()
        : hit(false)
        , distance(FLT_MAX)
        , point(0.0f)
        , normal(0.0f)
        , userData(nullptr)
    {}
};

// OSGWidget 的拾取方法实现
PickResult3D OSGWidget::pick(int x, int y)
{
    PickResult3D result;               // 最终返回结果，默认未命中
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return result;        // 无 Viewer 则直接返回

    osg::Camera* camera = viewer->getCamera();
    osg::Vec3f    nearPoint, farPoint;

    // 确保有有效的视口
    if (camera->getViewport())
    {
        // 构造：View * Projection * Viewport 矩阵
        osg::Matrix VPW = camera->getViewMatrix()
                        * camera->getProjectionMatrix()
                        * camera->getViewport()->computeWindowMatrix();
        osg::Matrix invVPW;
        invVPW.invert(VPW);  // 求逆，用于从窗口坐标映射到世界坐标

        // 获得点击来自窗口坐标（原点在右上角），绘制在opengl上（OpenGL 窗口原点在左下角），映射过程不是对称的，所以要翻转一次
        // 屏幕坐标 Y 轴翻转
        // 当 z=0 表示近裁剪面，z=1 表示远裁剪面
        nearPoint = osg::Vec3f(x, height() - y, 0.0f) * invVPW;
        farPoint  = osg::Vec3f(x, height() - y, 1.0f) * invVPW;
    }

    // 用 nearPoint 和 farPoint 构造射线
    Ray3D ray(
        glm::vec3(nearPoint.x(), nearPoint.y(), nearPoint.z()),
        glm::vec3(farPoint.x()  - nearPoint.x(),
                  farPoint.y()  - nearPoint.y(),
                  farPoint.z()  - nearPoint.z())
    );

    // 遍历场景中所有 Geo3D 对象，找出最近的交点
    // 可以使用八叉树优化，把3D对象放到完全被包围的最小体素里，然后射线穿过所有层的体素。
    // 可以判断对象少直接遍历
    float minDistance = FLT_MAX;
    for (Geo3D* geo : m_geoList)
    {
        PickResult3D geoResult;
        // 调用子类实现的 hitTest（通常先测包围盒，再做精细检测）
        if (geo->hitTest(ray, geoResult))
        {
            // 若距离更小，则更新结果
            if (geoResult.distance < minDistance)
            {
                minDistance = geoResult.distance;
                result = geoResult;
            }
        }
    }

    return result;  // 返回最近的拾取结果
}
```