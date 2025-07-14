#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include "../GeometryBase.h"
#include <osg/Camera>
#include <osg/FrameBufferObject>
#include <osg/Texture2D>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/StateSet>
#include <osg/Program>
#include <osg/Shader>
#include <osg/Uniform>
#include <osg/BufferObject>
#include <osg/TextureBuffer>
#include <osg/Image>
#include <memory>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <osgGA/GUIEventHandler>

// 64位ID编码结构
struct PickingID64
{
    static constexpr uint64_t OBJECT_ID_BITS = 40;     // 对象ID位数(支持1万亿对象)
    static constexpr uint64_t TYPE_CODE_BITS = 2;      // 类型代码位数
    static constexpr uint64_t LOCAL_IDX_BITS = 22;     // 本地索引位数(支持400万个Feature)
    
    static constexpr uint64_t OBJECT_ID_MASK = (1ULL << OBJECT_ID_BITS) - 1;
    static constexpr uint64_t TYPE_CODE_MASK = (1ULL << TYPE_CODE_BITS) - 1;
    static constexpr uint64_t LOCAL_IDX_MASK = (1ULL << LOCAL_IDX_BITS) - 1;
    
    enum TypeCode : uint8_t
    {
        TYPE_FACE = 0,      // 00 - 面
        TYPE_EDGE = 1,      // 01 - 边
        TYPE_VERTEX = 2,    // 10 - 点
        TYPE_INVALID = 3    // 11 - 无效
    };
    
    uint64_t objectID : OBJECT_ID_BITS;
    uint64_t typeCode : TYPE_CODE_BITS;
    uint64_t localIdx : LOCAL_IDX_BITS;
    
    PickingID64() : objectID(0), typeCode(TYPE_INVALID), localIdx(0) {}
    
    PickingID64(uint64_t objID, TypeCode type, uint64_t idx)
        : objectID(objID), typeCode(type), localIdx(idx) {}
    
    // 打包到64位整数
    uint64_t pack() const
    {
        return (objectID << (TYPE_CODE_BITS + LOCAL_IDX_BITS)) |
               (typeCode << LOCAL_IDX_BITS) |
               localIdx;
    }
    
    // 从64位整数解包
    void unpack(uint64_t packed)
    {
        localIdx = packed & LOCAL_IDX_MASK;
        typeCode = (packed >> LOCAL_IDX_BITS) & TYPE_CODE_MASK;
        objectID = (packed >> (TYPE_CODE_BITS + LOCAL_IDX_BITS)) & OBJECT_ID_MASK;
    }
    
    bool isValid() const { return typeCode != TYPE_INVALID; }
    
    // 转换为RGBA格式(用于GPU)
    osg::Vec4ub toRGBA() const
    {
        uint64_t packed = pack();
        return osg::Vec4ub(
            (packed >> 0) & 0xFF,    // R
            (packed >> 8) & 0xFF,    // G
            (packed >> 16) & 0xFF,   // B
            (packed >> 24) & 0xFF    // A
        );
    }
    
    // 从RGBA格式重建
    void fromRGBA(const osg::Vec4ub& rgba)
    {
        uint64_t packed = rgba.r() | (rgba.g() << 8) | (rgba.b() << 16) | (rgba.a() << 24);
        // 扩展为64位(这里假设高位部分存储在其他地方或为0)
        unpack(packed);
    }
    
    // 从RGBA格式重建(完整64位)
    void fromRGBA64(const osg::Vec4ub& rgba1, const osg::Vec4ub& rgba2)
    {
        uint64_t low = rgba1.r() | (rgba1.g() << 8) | (rgba1.b() << 16) | (rgba1.a() << 24);
        uint64_t high = rgba2.r() | (rgba2.g() << 8) | (rgba2.b() << 16) | (rgba2.a() << 24);
        unpack(low | (high << 32));
    }

    /// 返回当前的 TypeCode 枚举值
    inline TypeCode getTypeCode() const 
    {
        return static_cast<TypeCode>(typeCode);
    }
};

// 拾取对象数据
struct PickingObjectData
{
    osg::ref_ptr<Geo3D> geometry;           // 几何对象引用
    
    // 直接使用几何对象的点线面节点
    osg::ref_ptr<osg::Group> vertexGroup;   // 顶点组
    osg::ref_ptr<osg::Group> edgeGroup;     // 边组
    osg::ref_ptr<osg::Group> faceGroup;     // 面组
    
    // ID映射
    std::vector<PickingID64> vertexIDs;     // 顶点ID
    std::vector<PickingID64> edgeIDs;       // 边ID
    std::vector<PickingID64> faceIDs;       // 面ID
    
    PickingObjectData(Geo3D* geo) : geometry(geo) {}
};

// 拾取候选
struct PickingCandidate
{
    PickingID64 id;
    float depth;
    glm::vec3 worldPos;
    int screenX, screenY;
    
    PickingCandidate() : depth(1.0f), worldPos(0.0f), screenX(0), screenY(0) {}
    
    bool operator<(const PickingCandidate& other) const
    {
        // 按优先级排序：点 > 边 > 面，相同类型按深度排序
        if (id.typeCode != other.id.typeCode)
            return id.typeCode > other.id.typeCode;  // 点(2) > 边(1) > 面(0)
        return depth < other.depth;
    }
};

// 拾取结果
struct PickingResult
{
    bool hasResult;
    PickingID64 id;
    Geo3D* geometry;
    glm::vec3 worldPos;
    float depth;
    int screenX, screenY;
    
    PickingResult() : hasResult(false), geometry(nullptr), worldPos(0.0f), depth(1.0f), screenX(0), screenY(0) {}
};

// 拾取系统主类
class PickingSystem : public osg::Referenced
{
public:
    PickingSystem();
    virtual ~PickingSystem();
    
    // 初始化
    bool initialize(int width, int height);
    void resize(int width, int height);
    
    // 相机同步
    void syncWithMainCamera(osg::Camera* mainCamera);
    
    // 对象管理
    uint64_t addObject(Geo3D* geo);          // 返回分配的objectID
    void removeObject(uint64_t objectID);
    void removeObject(Geo3D* geo);           // 按几何对象删除
    void updateObject(uint64_t objectID);
    void updateObject(Geo3D* geo);           // 按几何对象更新
    void clearAllObjects();
    
    // 拾取操作
    PickingResult pick(int mouseX, int mouseY, int sampleRadius = 8);
    
    // 异步拾取(性能优化)
    void startAsyncPick(int mouseX, int mouseY, int sampleRadius = 8);
    bool isAsyncPickReady() const;
    PickingResult getAsyncPickResult();
    
    // 访问器
    osg::Camera* getPickingCamera() const { return m_pickingCamera.get(); }
    osg::Group* getPickingRoot() const { return m_pickingRoot.get(); }
    
    // 诊断接口
    bool isInitialized() const { return m_initialized; }
    int getObjectCount() const { return static_cast<int>(m_objectMap.size()); }
    int getFeatureCount() const;
    bool hasValidFrameBuffer() const { return m_fbo.valid() && m_colorTexture.valid() && m_depthTexture.valid(); }
    bool hasValidShaders() const { return m_pickingProgram.valid() && m_vertexShader.valid() && m_fragmentShader.valid(); }
    
    // 调试
    void setDebugMode(bool enabled) { m_debugMode = enabled; }
    bool isDebugMode() const { return m_debugMode; }
    void dumpPickingBuffer(const std::string& filename) const;

private:
    // 初始化相关
    bool createPickingCamera();
    bool createFrameBuffer();
    bool createShaders();
    void setupRenderStates();
    
    // 直接使用几何对象的点线面节点
    void setupPickingNodes(uint64_t objectID);
    void updatePickingNodes(uint64_t objectID);
    void rebuildPickingNodes(uint64_t objectID);
    
    // 渲染管线
    void renderPickingPass();
    void renderFaces();
    void renderEdges();
    void renderVertices();
    
    // 采样相关
    std::vector<PickingCandidate> sampleRegion(int centerX, int centerY, int radius);
    PickingCandidate samplePixel(int x, int y);
    PickingCandidate selectBestCandidate(const std::vector<PickingCandidate>& candidates);
    
    // 坐标转换
    glm::vec3 screenToWorld(int x, int y, float depth) const;
    
    // 异步处理
    void processAsyncPicking();
    
    // ID缓冲区上传
    void uploadIDBuffer();
    
private:
    // 基本配置
    int m_width, m_height;
    bool m_initialized;
    bool m_debugMode;
    
    // OSG组件
    osg::ref_ptr<osg::Camera> m_pickingCamera;
    osg::ref_ptr<osg::Group> m_pickingRoot;
    osg::ref_ptr<osg::Group> m_faceGroup;
    osg::ref_ptr<osg::Group> m_edgeGroup;
    osg::ref_ptr<osg::Group> m_vertexGroup;
    
    // FBO相关
    osg::ref_ptr<osg::FrameBufferObject> m_fbo;
    osg::ref_ptr<osg::Texture2D> m_colorTexture;
    osg::ref_ptr<osg::Texture2D> m_depthTexture;
    osg::ref_ptr<osg::Image> m_colorImage;
    osg::ref_ptr<osg::Image> m_depthImage;
    
    // 着色器
    osg::ref_ptr<osg::Program> m_pickingProgram;
    osg::ref_ptr<osg::Shader> m_vertexShader;
    osg::ref_ptr<osg::Shader> m_fragmentShader;
    
    // ID缓冲区
    osg::ref_ptr<osg::TextureBuffer> m_idBuffer;
    osg::ref_ptr<osg::UIntArray> m_idBufferData;
    std::vector<uint64_t> m_idArray;
    
    // 对象管理
    std::unordered_map<uint64_t, PickingObjectData> m_objectMap;      // objectID -> 拾取数据
    std::unordered_map<Geo3D*, uint64_t> m_geoToIDMap;               // 几何对象 -> objectID
    uint64_t m_nextObjectID;
    
    // 异步拾取
    bool m_asyncPickingEnabled;
    bool m_asyncPickingInProgress;
    bool m_asyncPickingReady;
    PickingResult m_asyncResult;
    
    // 双缓冲PBO(性能优化) - OSG 3.6.5兼容
    osg::ref_ptr<osg::Image> m_readImage[2];
    int m_currentPBO;
    
    // 主相机引用
    osg::ref_ptr<osg::Camera> m_mainCamera;
    
    // 频率限制
    double m_lastPickTime;
    double m_pickFrequencyLimit;  // 秒
    
    // 性能统计
    mutable double m_avgPickTime;
    mutable int m_pickCount;
};

// 拾取系统管理器(单例)
class PickingSystemManager
{
public:
    static PickingSystemManager& getInstance();
    
    PickingSystem* getPickingSystem() const { return m_pickingSystem.get(); }
    
    bool initialize(int width, int height);
    void setMainCamera(osg::Camera* camera);
    
    // 便捷接口
    uint64_t addObject(Geo3D* geo);     // 返回分配的ID
    void removeObject(Geo3D* geo);
    void updateObject(Geo3D* geo);
    PickingResult pick(int mouseX, int mouseY, int sampleRadius = 8);

private:
    PickingSystemManager();
    ~PickingSystemManager();
    
    PickingSystemManager(const PickingSystemManager&) = delete;
    PickingSystemManager& operator=(const PickingSystemManager&) = delete;
    
    osg::ref_ptr<PickingSystem> m_pickingSystem;
};

// 拾取事件处理器
class PickingEventHandler : public osgGA::GUIEventHandler
{
public:
    PickingEventHandler();
    virtual ~PickingEventHandler();
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override;
    
    // 设置回调
    void setPickingCallback(std::function<void(const PickingResult&)> callback);
    
    // 配置
    void setPickingRadius(int radius) { m_pickingRadius = radius; }
    void setPickingFrequency(double frequency) { m_pickingFrequency = frequency; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

private:
    void processPicking(int x, int y);
    
    std::function<void(const PickingResult&)> m_pickingCallback;
    int m_pickingRadius;
    double m_pickingFrequency;
    double m_lastPickTime;
    bool m_enabled;
    int m_lastX, m_lastY;
}; 