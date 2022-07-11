#include "ofApp.h"
#include <vector> // calcTangents() 함수에서 동적 배열인 std::vector 컨테이너 클래스 템플릿을 사용하기 위해 해당 클래스를 include 시킴.

// 탄젠트 벡터 계산 후, 메쉬의 버텍스 컬러 데이터에 임시로 저장해두는 함수
void calcTangents(ofMesh& mesh) { // mesh 함수인자는 'ofMesh' 타입을 참조한다는 뜻. (diffuse-lighting 레포지토리 참조자 관련 설명 참고)
    using namespace glm;
    std::vector<vec4> tangents; // c++ 에 정의된 표준 라이브러리 사용 시, std:: 접두어를 항상 붙여주도록 함. 여기서는 동적 배열인 std::vector 컨테이너 클래스 템플릿을 사용함.
    tangents.resize(mesh.getNumVertices()); // 버텍스 개수만큼 탄젠트 벡터를 담는 동적 배열의 길이 생성
    
    uint indexCount = mesh.getNumIndices(); // 버텍스의 인덱스 개수만큼 인덱스 카운트 개수 지정
    
    const vec3* vertices = mesh.getVerticesPointer();
    const vec2* uvs = mesh.getTexCoordsPointer();
    const uint* indices = mesh.getIndexPointer();
    
    for (uint i = 0; i < indexCount - 2; i += 3) {
        // 3개의 인접한 버텍스들 -> 즉, 하나의 삼각형 안의 3개의 버텍스들의 위치데이터와 uv데이터를 각각 구함
        // 왜 삼각형일까? 삼각형만이 온전한 평면을 구성하고, 같은 평면안에 존재한다면 탄젠트 공간 내의 탄젠트 벡터도 동일한 걸 공유한다고 가정하기 때문인 것 같음.
        // 왜냐, 같은 평면 안에 존재하는 버텍스라면, 그 버텍스들은 동일한 노말벡터를 공유하기 때문에, 탄젠트 벡터도 동일하다고 봄.
        const vec3& v0 = vertices[indices[i]];
        const vec3& v1 = vertices[indices[i + 1]];
        const vec3& v2 = vertices[indices[i + 2]];
        const vec2& uv0 = uvs[indices[i]];
        const vec2& uv1 = uvs[indices[i + 1]];
        const vec2& uv2 = uvs[indices[i + 2]];
        
        // v0 버텍스에 연결된 2개의 선분(벡터)을 구함 -> 3차원 공간 상의 선분(벡터)
        vec3 edge1 = v1 - v0; // 선분(벡터)1
        vec3 edge2 = v2 - v0; // 선분(벡터)2
        // v0 버텍스의 uv좌표에 연결된 uv좌표계 상의 2개의 선분(벡터)도 구함 (아마 distance UV 의 줄임말인 듯.) -> 2차원 공간(uv좌표계) 상의 선분(벡터)
        vec2 dUV1 = uv1 - uv0; // uv좌표계 선분1
        vec2 dUV2 = uv2 - uv0; // uv좌표계 선분2
        
        // 위에서 구한 두 벡터를 기저 벡터로 삼아(?) 탄젠트 벡터를 구하는 과정같음.. (아직 자세히는 모르겠음ㅜ)
        float f = 1.0f / (dUV1.x * dUV2.y - dUV2.x * dUV1.y);
        
        vec4 tan;
        tan.x = f * (dUV2.y * edge1.x - dUV1.y * edge2.x);
        tan.y = f * (dUV2.y * edge1.y - dUV1.y * edge2.y);
        tan.z = f * (dUV2.y * edge1.z - dUV1.y * edge2.z);
        tan.w = 0;
        tan = normalize(tan); // 계산한 탄젠트 벡터를 정규화하여 길이를 1로 맞춤.
        
        // 서로 인접하는 3개의 버텍스, 즉 한 삼각형 안의 버텍스들은 모두 동일한 탄젠트 벡터를 갖도록
        // tangents 동적배열에 인접한 3개의 인덱스에 동일한 탄젠트 벡터를 넣어줌..
        tangents[indices[i]] += (tan);
        tangents[indices[i + 1]] += (tan);
        tangents[indices[i + 2]] += (tan);
    }
    
    int numColors = mesh.getNumColors(); // 오픈프레임웍스가 버텍스 탄젠트 데이터를 지원하지 않아서 버텍스 컬러데이터에 대신 넣어주려는 것.

    for (int i = 0; i < tangents.size(); ++i) {
        vec3 t = normalize(tangents[i]); // 위에 반복문에서 저장해 둔 각 인덱스의 탄젠트 벡터를 가져온 뒤, vec3에 할당함으로써 각 탄젠트벡터의 x, y, z 컴포넌트만 가져옴.
        
        // 탄젠트벡터 데이터가 컬러데이터와 개수가 안맞을 수 있기 때문에,
        // i가 전체 컬러데이터 수보다 적으면 기존 i번쩨 컬러데이터를 setColor() 로 탄젠트 벡터 데이터로 덮어쓰고,
        // i가 전체 컬러데이터 수보다 크다면, addColor() 로 탄젠트 벡터 데이터를 전달해서 컬러데이터를 새로 추가함.
        if (i >= numColors) {
            mesh.addColor(ofFloatColor(t.x, t.y, t.z, 0.0));
        } else {
            mesh.setColor(i, ofFloatColor(t.x, t.y, t.z, 0.0));
        }
    }
}

// 조명계산 최적화를 위해, 쉐이더에서 반복계산하지 않도록, c++ 에서 한번만 계산해줘도 되는 작업들을 수행하는 보조함수들
glm::vec3 getLightDirection(DirectionalLight& l) {
    // 조명벡터 direction에 -1을 곱해서 조명벡터의 방향을 뒤집어주고, 셰이더에서 내적계산을 해주기 위해 길이를 1로 정규화해서 맞춰줌.
    return glm::normalize(l.direction * -1.0f);
}

glm::vec3 getLightColor(DirectionalLight& l) {
    // 디렉셔널라이트 구조체에서 vec3 값인 조명색상에 float 값인 조명강도를 스칼라배로 곱해줘서 조명색상의 밝기를 지정함.
    return l.color * l.intensity;
}

glm::vec3 getLightColor(PointLight& l) {
    // 포인트라이트 구조체에서 vec3 값인 조명색상에 float 값인 조명강도를 스칼라배로 곱해줘서 조명색상의 밝기를 지정함.
    return l.color * l.intensity;
}

//--------------------------------------------------------------
void ofApp::setup(){
    ofDisableArbTex(); // 스크린 픽셀 좌표를 사용하는 텍스쳐 관련 오픈프레임웍스 레거시 지원 설정 비활성화. (uv좌표계랑 다르니까!)
    ofEnableDepthTest(); // 깊이테스트를 활성화하여 z버퍼에 저장해서 각 요소에서 카메라와의 거리를 기준으로 앞뒤를 구분하여 렌더링할 수 있도록 함.
        
    // 이번에는 ofApp 맨 처음 설정에서 shieldMesh 를 바라보기 적당한 카메라 위치와 시야각을 지정함.
    cam.pos = glm::vec3(0, 0.75f, 1.0f); // 카메라 위치는 z축으로 1.0만큼 안쪽으로 들어가게 하고, 조명 연산 결과를 확인하기 위해 y축으로도 살짝 올려줌
    cam.fov = glm::radians(90.0f); // 원근 프러스텀의 시야각은 일반 PC 게임에서는 90도 전후의 값을 사용함. -> 라디안 각도로 변환하는 glm 내장함수 radians() 를 사용함.
        
    planeMesh.load("plane.ply"); // planeMesh 메쉬로 사용할 모델링 파일 로드
    calcTangents(planeMesh); // plane 메쉬의 버텍스들을 이용해서 각 버텍스의 탄젠트 벡터를 구한 뒤 버텍스 컬러데이터 자리에 저장하는 함수 실행
        
    shieldMesh.load("shield.ply"); // shieldMesh 메쉬로 사용할 모델링 파일 로드
    calcTangents(shieldMesh); // shield 메쉬에 탄젠트 벡터를 구한 뒤 버텍스 컬러 자리에 저장하는 함수 실행
    
    cubeMesh.load("cube.ply"); // cubeMesh 메쉬로 사용할 모델링 파일 로드

    dirLightShaders[0].load("mesh.vert", "dirLight.frag"); // 방패메쉬에 적용할 디렉셔널 라이트 쉐이더 파일 로드
    pointLightShaders[0].load("mesh.vert", "pointLight.frag"); // 방패메쉬에 적용할 포인트라이트 쉐이더 파일 로드
    
    dirLightShaders[1].load("water.vert", "dirLightWater.frag"); // plane 메쉬에 적용할 디렉셔널 라이트 쉐이더 파일 로드
    pointLightShaders[1].load("water.vert", "pointLightWater.frag"); // plane 메쉬에 적용할 포인트라이트 쉐이더 파일 로드
    
    skyboxShader.load("skybox.vert", "skybox.frag"); // cubeMesh 에 큐브맵 텍스쳐를 적용한 셰이더를 적용하기 위한 셰이더 파일 로드
        
    waterNrm.load("water_nrm.png"); // planeMesh 의 조명계산에서 노말맵으로 사용할 텍스쳐 로드
    waterNrm.getTexture().setTextureWrap(GL_REPEAT, GL_REPEAT); // 프래그먼트 셰이더에서 노말맵 텍스쳐를 타일링하여 샘플링할 것이므로, 노말맵의 랩 모드를 반복으로 지정함.
        
    diffuseTex.load("shield_diffuse.png"); // shieldMesh 의 조명계산에서 디퓨즈 라이팅 계산에 사용할 텍스쳐 로드
    specTex.load("shield_spec.png"); // shieldMesh 의 조명계산에서 스펙큘러 라이팅 계산에 사용할 텍스쳐 로드
    nrmTex.load("shield_normal.png"); // shieldMesh 의 조명계산에서 노말맵으로 사용할 텍스쳐 로드
    
    // 큐브맵 텍스쳐를 로드함.
    cubemap.load("night_front.jpg", "night_back.jpg",
            "night_right.jpg", "night_left.jpg",
            "night_top.jpg", "night_bottom.jpg");
    
    // 이전 예제들과 다르게 draw() 함수가 아닌 setup() 함수에서 조명구조체에 조명데이터를 할당해 줌.
    // 근데 사실 생각하면 원래부터 setup() 함수에서 세팅을 해줘야하는게 맞음. 조명데이터를 draw() 함수에서 반복적으로 할당해줄 필요는 없으니까...
    // 원하는 개수만큼 포인트라이트 구조체를 생성하여 조명데이터 할당.
    PointLight pl0;
    pl0.color = glm::vec3(1, 0, 0);
    pl0.radius = 1.0f;
    pl0.position = glm::vec3(-0.5, 0.35, 0.25);
    pl0.intensity = 3.0f;
    
    PointLight pl1;
    pl1.color = glm::vec3(0, 1, 0);
    pl1.radius = 1.0f;
    pl1.position = glm::vec3(0.5, 0.35, 0.25);
    pl1.intensity = 3.0f;
    
    PointLight pl2;
    pl2.color = glm::vec3(0, 0, 1);
    pl2.radius = 1.0f;
    pl2.position = glm::vec3(0.0, 0.7, 0.25);
    pl2.intensity = 3.0f;
    
    // 원하는 개수만큼 생성한 포인트라이트 구조체를 pointLights 동적배열에 push함
    // 참고로, 동적배열.push_back() 은 요소를 동적배열 맨뒤에 추가할 때 사용하는 함수임.
    pointLights.push_back(pl0);
    pointLights.push_back(pl1);
    pointLights.push_back(pl2);
    
    // 디렉셔널 라이트 구조체 생성하여 조명데이터 할당
    dirLight.color = glm::vec3(1, 1, 0);
    dirLight.intensity = 0.25f;
    dirLight.direction = glm::vec3(0, 0, -1);
}

//--------------------------------------------------------------
void ofApp::update(){

}

// waterMesh 의 각종 변환행렬을 계산한 뒤, 유니폼 변수들을 전송해주면서 드로우콜을 호출하는 함수
void ofApp::drawWater(Light& light, glm::mat4& proj, glm::mat4& view) {
    using namespace glm;
    
    static float t = 0.0f; // static 을 특정 함수 내에서 사용하는 것을 '정적 지역 변수'라고 하며, 이 할당문은 drawWater() 함수 최초 호출 시 1번만 실행됨.
    t += ofGetLastFrameTime(); // 이전 프레임과 현재 프레임의 시간 간격인 '델타타임'을 리턴받는 함수를 호출해서 유니폼 변수로 전송할 시간값 t에 매 프레임마다 더해줌.
    
    // waterMesh 의 모델행렬 계산 (회전행렬 및 크기행렬만 적용)
    vec3 right = vec3(1, 0, 0); // waterMesh 모델의 회전행렬을 계산할 시, x축 방향으로만 회전할 수 있도록 회전축 벡터를 구해놓음.
    mat4 rotation = rotate(radians(-90.0f), right); // waterMesh 회전행렬은 x축 기준으로 cAngle(-90도) 회전시킴.
    mat4 model = rotation * scale(vec3(5.0, 4.0, 4.0)); // 열 우선 행렬이므로, 원하는 행렬 곱셈과 반대순서인 회전행렬 * 크기행렬 순으로 곱해줌
    
    // 최적화를 위해 c++ 단에서 투영 * 뷰 * 모델행렬을 한꺼번에 곱해서 버텍스 셰이더에 전송함.
    mat4 mvp = proj * view * model; // 열 우선 행렬이라 원래의 곱셈 순서인 '모델 -> 뷰 -> 투영'의 반대 순서로 곱해줘야 함.
    
    /**
         모델의 버텍스가 갖고있는 기본 노말벡터는 오브젝트공간을 기준으로 되어있음.
         그러나, 조명계산을 하려면 이러한 노말벡터를 월드공간으로 변환해야 함.
                 
         그럼 노말벡터도 그냥 모델행렬인 mat4 model 을 버텍스 셰이더에서 곱해주면 되는거 아닌가?
         이렇게 할 수도 있지만, 만약 모델행렬에 '비일률적 크기행렬' (예를들어, (0.5, 1.0, 1.0) 이런거)
         가 적용되어 있다면, 특정 축마다 scale 이 다르게 늘어나는 과정에서 노말벡터의 방향이 휘어버리게 됨. -> p.190 참고
                 
         이걸 똑바르게 세워주려면, '노멀행렬' 이라는 새로운 행렬이 필요함.
         노말행렬은 '모델행렬의 상단 3*3 역행렬의 전치행렬' 로 정의할 수 있음.
                 
         역행렬, 전치행렬, 상단 3*3 행렬에 대한 각각의 개념은 위키백과, 구글링, 북마크한거 참고...
                 
         어쨋든 위의 정의에 따라 아래와 같이 노말행렬을 구하고, 버텍스 셰이더로 쏴주면 됨.
    */
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    
    // 인자로 받아온 Light 구조체의 isPointLight() 함수 리턴값에 따라 포인트라이트 셰이더 또는 디렉셔널라이트 셰이더 중에서 참조자 shd 가 어떤 셰이더 객체를 참조하도록 할 지 결정함.
    ofShader shd = light.isPointLight() ? pointLightShaders[1] : dirLightShaders[1];
    
    // shd 를 바인딩하여 사용 시작
    shd.begin();
    light.apply(shd); // 인자로 전달받는 각 조명구조체는 부모구조체 Light 로부터 상속받은 apply 함수에서 유니폼 변수에 자신의 멤버변수 값을 전송하는 로직이 override 되어있음. 이걸 여기서 호출함으로써 유니폼 변수에 멤버변수값을 전송하려는 것.
    
    shd.setUniformMatrix4f("mvp", mvp); // 위에서 한꺼번에 합쳐준 mvp 행렬을 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniformMatrix4f("model", model); // 버텍스 좌표를 월드좌표로 변환하기 위해 모델행렬만 따로 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniformMatrix3f("normalMatrix", normalMatrix); // 노말행렬을 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniform3f("meshSpecCol", glm::vec3(1, 1, 1)); // 스펙큘러 색상을 흰색으로 지정하여 유니폼 변수로 전송
    shd.setUniformTexture("normTex", waterNrm, 0); // 노말 매핑에 사용할 텍스쳐 유니폼 변수로 전송
    shd.setUniformTexture("envMap", cubemap.getTexture(), 1); // 환경맵 반사를 적용하기 위해 사용할 큐브맵 텍스쳐 유니폼 변수로 전송
    shd.setUniform1f("time", t); // uv 스크롤링에 사용할 시간값 유니폼 변수로 전송
    shd.setUniform3f("ambientCol", glm::vec3(0.0, 0.0, 0.0)); // 환경광으로 사용할 앰비언트 라이트 색상값을 유니폼 변수로 전송.
    shd.setUniform3f("cameraPos", cam.pos); // 프래그먼트 셰이더에서 뷰 벡터를 계산하기 위해 카메라 좌표(카메라 월드좌표)를 프래그먼트 셰이더 유니폼 변수로 전송
    
    planeMesh.draw(); // planeMesh(waterMesh) 메쉬 드로우콜 호출하여 그려줌.
    
    shd.end();
    // shd 사용 중단
}

void ofApp::drawSkybox(glm::mat4& proj, glm::mat4& view) {
    using namespace glm; // 이제부터 이 함수블록 내에서 glm 라이브러리에서 꺼내 쓸 함수 및 객체들은 'glm::' 을 생략해서 사용해도 됨.
    
    // cubeMesh 의 모델행렬 계산 (이동행렬만 적용)
    mat4 model = translate(cam.pos); // 큐브맵의 위치(즉, 큐브맵의 오브젝트공간 원점)를 카메라 위치와 일치시킴으로써, 큐브맵 안쪽 중앙에 카메라가 위치하도록 함 -> 큐브맵을 skybox 로 만들기 위함!
    
    // 최적화를 위해 c++ 단에서 투영 * 뷰 * 모델행렬을 한꺼번에 곱해서 버텍스 셰이더에 전송함.
    mat4 mvp = proj * view * model; // 열 우선 행렬이라 원래의 곱셈 순서인 '모델 -> 뷰 -> 투영'의 반대 순서로 곱해줘야 함.
    
    ofShader& shd = skyboxShader; // 참조자 shd 는 ofShader 타입의 멤버변수 skyboxShader 를 참조하도록 함.
    
    glDepthFunc(GL_LEQUAL); // 스카이박스를 그리기 전 깊이비교모드를 변경함 (깊이비교모드 관련 필기 하단 참고) Less Equal 의 줄임말. 즉, >= (보다 작거나 같음. 이하)을 의미
    
    // shd(skyboxShader 를 참조) 를 바인딩하여 사용 시작
    shd.begin();
    shd.setUniformMatrix4f("mvp", mvp); // 위에서 한꺼번에 합쳐준 mvp 행렬을 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniformTexture("envMap", cubemap.getTexture(), 0); // 커스텀 큐브맵 클래스로 셰이더의 유니폼 변수에 큐브맵 텍스쳐를 전송할 경우, 명시적으로 getTexture() 를 호출해야 함.
    
    cubeMesh.draw(); // cubeMesh 메쉬 드로우콜 호출하여 그림.
    
    shd.end();
    // shd(cubemapShader) 사용 중단
    
    glDepthFunc(GL_LESS); // 스카이박스를 다 그린 뒤 깊이비교모드를 원래대로 원상복구함. (깊이비교모드 관련 필기 하단 참고) Less 의 줄임말. 즉, > (보다 작음. 미만)을 의미
}

void ofApp::drawShield(Light& light, glm::mat4& proj, glm::mat4& view) {
    using namespace glm;
    
    mat4 model = translate(vec3(0.0, 0.75, 0.0f)); // shieldMesh 의 모델행렬 계산 (이동행렬만 적용)
    mat4 mvp = proj * view * model; // 최적화를 위해 c++ 단에서 투영 * 뷰 * 모델행렬을 한꺼번에 곱해서 버텍스 셰이더에 전송함.
    mat3 normalMatrix = mat3(transpose(inverse(model))); // 노말행렬은 '모델행렬의 상단 3*3 역행렬의 전치행렬' 로 계산함.
    
    // 인자로 받아온 Light 구조체의 isPointLight() 함수 리턴값에 따라 포인트라이트 셰이더 또는 디렉셔널라이트 셰이더 중에서 참조자 shd 가 어떤 셰이더 객체를 참조하도록 할 지 결정함.
    ofShader shd = light.isPointLight() ? pointLightShaders[0] : dirLightShaders[0];

    // shd 를 바인딩하여 사용 시작
    shd.begin();
    light.apply(shd); // 인자로 전달받는 각 조명구조체는 부모구조체 Light 로부터 상속받은 apply 함수에서 유니폼 변수에 자신의 멤버변수 값을 전송하는 로직이 override 되어있음. 이걸 여기서 호출함으로써 유니폼 변수에 멤버변수값을 전송하려는 것.

    shd.setUniformMatrix4f("mvp", mvp); // 위에서 한꺼번에 합쳐준 mvp 행렬을 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniformMatrix4f("model", model); // 버텍스 좌표를 월드좌표로 변환하기 위해 모델행렬만 따로 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniformMatrix3f("normalMatrix", normalMatrix); // 노말행렬을 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniform3f("meshSpecCol", glm::vec3(1, 1, 1)); // 스펙큘러 색상을 흰색으로 지정하여 유니폼 변수로 전송
    shd.setUniformTexture("diffuseTex", diffuseTex, 0); // 디퓨즈 라이팅 계산에 사용할 텍스쳐 유니폼 변수로 전송
    shd.setUniformTexture("specTex", specTex, 1); // 스펙큘러 라이팅 계산에 사용할 텍스쳐 유니폼 변수로 전송
    shd.setUniformTexture("nrmTex", nrmTex, 2); // 노말 매핑에 사용할 텍스쳐 유니폼 변수로 전송
    shd.setUniformTexture("envMap", cubemap.getTexture(), 3); // 환경맵 반사를 적용하기 위해 사용할 큐브맵 텍스쳐 유니폼 변수로 전송
    shd.setUniform3f("ambientCol", glm::vec3(0.0, 0.0, 0.0)); // 배경색과 동일한 앰비언트 라이트 색상값을 유니폼 변수로 전송.
    shd.setUniform3f("cameraPos", cam.pos); // 프래그먼트 셰이더에서 뷰 벡터를 계산하기 위해 카메라 좌표(카메라 월드좌표)를 프래그먼트 셰이더 유니폼 변수로 전송
    
    shieldMesh.draw(); // shieldMesh 메쉬 드로우콜 호출하여 그려줌.
    
    shd.end();
    // shd 사용 중단
}

// 포인트라이트 패스 렌더링 시, 블렌딩모드와 깊이테스트 모드를 재설정하는 함수
void ofApp::beginRenderingPointLights() {
    // 동적 멀티라이팅 기법에서는 멀티패스 셰이딩, 즉 물체 하나에 여러 개의 셰이더가 적용된 동일한 메쉬를 반복해서 그려주는 방식을 사용함.
    // 따라서, 이전에 그려진 동일한 메쉬의 색상(기존 버퍼 색상)과 새로 그려진 동일한 메쉬(새 색상)을 가산 블렌딩할 수 있도록 활성화한 것.
    ofEnableAlphaBlending();
    ofEnableBlendMode(ofBlendMode::OF_BLENDMODE_ADD);
    
    // 깊이 테스트 모드를 GL_LEQUAL 로 변경함. (관련 설명 하단 필기 참고)
    glDepthFunc(GL_LEQUAL);
}

// 포인트라이트 패스 렌더링 완료 후, 블렌딩모드와 깊이테스트 모드를 원래대로 초기화하는 함수
void ofApp::endRenderingPointLights() {
    // 알파블렌딩 모드를 비활성화함.
    ofDisableAlphaBlending();
    ofDisableBlendMode();
    
    // 깊이테스트 모드를 원래의 GL_LESS 로 초기화함.
    glDepthFunc(GL_LESS);
}

//--------------------------------------------------------------
void ofApp::draw(){
    using namespace glm; // 이제부터 현재 블록 내에서 glm 라이브러리에서 꺼내 쓸 함수 및 객체들은 'glm::' 을 생략해서 사용해도 됨.
        
    // 투영행렬 계산
    float aspect = 1024.0f / 768.0f; // main.cpp 에서 정의한 윈도우 실행창 사이즈를 기준으로 원근투영행렬의 종횡비(aspect)값을 계산함.
    mat4 proj = perspective(cam.fov, aspect, 0.01f, 10.0f); // glm::perspective() 내장함수를 사용해 원근투영행렬 계산.
    
    // 카메라 변환시키는 뷰행렬 계산. 이동행렬만 적용
    mat4 view = inverse(translate(cam.pos)); // 뷰행렬은 카메라 움직임에 반대방향으로 나머지 대상들을 움직이는 변환행렬이므로, glm::inverse() 내장함수로 역행렬을 구해야 함.
    
    drawSkybox(proj, view); // cubeMesh 메쉬 드로우 함수를 추출하여 정의한 뒤 호출함.

    // 이제 동일한 방패메쉬 및 물 메쉬에 대해 여러 개의 멀티패스 셰이딩이 적용된 메쉬들을 반복적으로 렌더링함.
    // 디렉셔널 라이트 셰이더가 적용된 방패메쉬 및 물 메쉬 렌더링함.
    drawWater(dirLight, proj, view);
    drawShield(dirLight, proj, view);

    // 포인트라이트 셰이더가 적용된 방패메쉬 및 물 메쉬 렌더링함.
    // 이때, 이전에 그린 방패메쉬 및 물 메쉬의 프래그먼트들과 색상을 가산블렌딩하기 위해 알파블렌딩 및 깊이테스트 설정을 변경함
    beginRenderingPointLights();

    // 포인트라이트 구조체가 담긴 동적배열을 for loop 로 돌리면서
    // 원하는 개수만큼의 포인트라이트 셰이더가 적용된 방패메쉬 및 물 메쉬를 반복해서 렌더링함.
    for (int i = 0; i < pointLights.size(); ++i) {
        drawWater(pointLights[i], proj, view);
        drawShield(pointLights[i], proj, view);
    }

    // 포인트라이트가 적용된 방패메쉬 및 물 메쉬 렌더링이 모두 끝나면, 알파블렌딩 및 깊이테스트 관련 설정을 초기화함.
    endRenderingPointLights();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}

/**
 스카이박스 원근분할 무효화를 위한 깊이비교모드 변경
 
 스카이박스는 씬에서 볼 수 있는 대상 중 가장 멀리있는 대상이지만,
 투영공간에서 유일하게 원근분할 ( 원근 분할 설명은 p.260 ~ 261 참고) 이 적용이 안되어야 함.
 왜냐면, 스카이박스는 무한히 뻗어나가는 하늘을 보여줘야 하기 때문에, 여기에 원근효과를 적용하는 건 적절치 않음.
 
 따라서, 스카이박스는 버텍스 셰이더에서 w컴포넌트와 z컴포넌트를 일치시켜 원근분할을 무효화하는데,
 이 때, 원근분할로 변환된 NDC 좌표계 상에서는 기본적으로 좌표값이 1.0 보다 작은 버텍스만 그릴 수 있게 되어있음.
 (이러한 깊이 모드를 GL_LESS 라고 함.)
 
 그런데, 스카이박스는 Z / W = 1.0 이기 때문에
 GL_LESS 깊이비교모드 상에서는 화면에 그려지지 않을거임.
 
 따라서, OpenGL 함수인 glDepthFunc() 를 통해 깊이비교모드를
 GL_LEQUAL 로 일시적으로 변경함으로써,
 NDC 좌표계 상에서 z값이 1.0보다 같거나 작은 대상까지 그리게 하도록 변경함.
 
 이렇게 하면 스카이박스까지는 그려질 거니까 괜찮지만,
 일반적으로 사용하는 깊이비교모드는 아니기 때문에,
 혹시나 다른 부분에서 문제가 발생할 여지를 줄이기 위해
 
 스카이박스를 전부 그리고 나면 맨 마지막에
 깊이비교모드를 GL_LESS 로 원상복귀시킴.
 */

/**
 GL_LESS vs GL_LEQUAL 의 기본 원리
 
 원론적으로 이야기하면,
 GL_LESS 는 기본적으로 이전에 그렸던 프래그먼트의
 깊이값보다 지금 그리는 프랙먼트의 깊이값이 더 작아야만 통과를 시킴.
 
 이는 모든 깊이테스트의 기본모드이기 때문에,
 이 상태에서 멀티패스 셰이딩으로 동일한 자리에
 동일한 방패메쉬를 여러 번 그리고자 한다면,
 
 방패메쉬의 프래그먼트들은 모두 깊이값이 동일하니까
 통과를 못하고 그려지지 않는 것임.
 
 그래서 임시로 깊이테스트 모드를 GL_LEQUAL 모드로 변경해서
 '이전 프래그먼트의 깊이값보다 같거나 작은 깊이값' 을 갖는 프래그먼트들도
 전부 통과되서 그려질 수 있도록 한 것임.
 
 이 원리를 이해하면, 위에서
 원근분할 시 왜 GL_LEQUAL 을 사용한 것인지도 이해가 감.
 
 왜냐하면, 스카이박스의 Z / W 값은 모두 1.0이기 때문에,
 깊이값이 모두 1.0이 된다는 뜻임.
 
 이 말은, 앞서 그려지는 깊이값이 1.0인  프래그먼트들에 의해서
 스카이박스가 그려지지 않는 문제를 해결하기 위해
 1.0보다 같거나 작은 깊이값을 갖는 프래그먼트들(즉, 스카이박스의 프래그먼트들)도
 모두 화면에 그려질 수 있도록 GL_LEQUAL 로 깊이테스트 모드를 변경한 것임!
 */

/**
 #define 매크로 상수
 
 c++ 에서는 프로그램 내에서 컴파일러가
 매크로를 만났을 때 대체할 문자열 또는 숫자값을 정의할 때 사용함.
 
 컴파일러가 컴파일 시작 전,
 #define 으로 정의된 기호를 지정된 숫자나 문자열로
 변환시킬 수 있도록 함.
 
 매크로는 변수가 아니므로, 런타임 이전에
 컴파일러가 해당 기호를 마주치면 지정된 값으로
 변환해주는 형태이므로 변수보다 처리속도가 빠르고,
 코드의 가독성 및 유지보수를 향상시키는 이점이 있다고 함.
 */
