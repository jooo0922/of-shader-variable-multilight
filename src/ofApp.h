#pragma once

#include "ofMain.h"
#include "ofxEasyCubemap.hpp"
#include <vector> // 동적 배열을 사용하기 위해 std::vector c++ 표준 라이브러리를 사용하기 위해 해당 템플릿을 include 시킴.

// 카메라의 현재 위치 및 fov(시야각)값을 받는 구조체 타입 지정. (구조체 타입은 ts interface 랑 비슷한 개념이라고 생각하면 될 것 같음.)
struct CameraData {
    glm::vec3 pos;
    float fov;
};

// DirectionalLight 와 PointLight 구조체를 다형성으로 상속시킬 때 사용할 부모 구조체 Light 정의
struct Light {
    virtual bool isPointLight() { // virtual 키워드로 가상함수 정의 (가상함수 관련 필기 하단 참고)
        // ofApp.cpp 의 drawWater(), drawShield() 에서 인자로 전달받은 조명구조체가 포인트라이트인지 아닌지 여부를 체크하는 메서드
        // 기본값은 false, 즉 포인트라이트가 아님을 의미하는 불리언 값을 리턴함.
        return false;
    }
    virtual void apply(ofShader& shd) {
        // 상속받는 구조체에서 각 구조체에 포함된 멤버변수들을 유니폼 변수로 전송하는 로직들이 override 될 것임.
        // drawWater(), drawShield() 내에서 호출됨.
    };
};

// Light 부모 구조체로부터 상속받은 뒤, 각 조명구조체에 따라 세부사항들을 따로 추가하거나 override 하고 있음 -> '다형성' 으로 구성됨.
// ofApp.cpp 에서 유니폼 변수에 값을 전송하는 렌더링 로직을 단순화하기 위해 이렇게 구성했다고 함.
struct DirectionalLight : public Light {
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    
    // 자식 구조체의 멤버 함수에도 virtual 을 붙여서 부모 구조체의 가상함수로부터 override 한 것임을 명시하는 게 좋음.
    virtual void apply(ofShader& shd) override {
        // 구조체의 멤버변수 값들을 셰이더 코드의 유니폼 변수로 전송하는 로직들로 override 해줌.
        shd.setUniform3f("lightDir", -direction);
        shd.setUniform3f("lightCol", color * intensity);
    }
};

struct PointLight : public Light { // 상속 접근 지정자를 public 으로 지정함. (상속 접근 지정자 관련 필기 하단 참고)
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    float radius;
    
    virtual bool isPointLight() override {
        // 포인트라이트 여부를 체크하는 부모구조체의 가상함수를 override 해서 true를 리턴하도록 함. (이 구조체는 포인트라이트 구조체니까 당연하지?)
        return true;
    }
    virtual void apply(ofShader& shd) override {
        // 구조체의 멤버변수 값들을 셰이더 코드의 유니폼 변수로 전송하는 로직들로 override 해줌.
        shd.setUniform3f("lightPos", position);
        shd.setUniform3f("lightCol", color * intensity);
        shd.setUniform1f("lightRadius", radius);
    }
};

class ofApp : public ofBaseApp{

    public:
        void setup();
        void update();
        void draw();

        void keyPressed(int key);
        void keyReleased(int key);
        void mouseMoved(int x, int y );
        void mouseDragged(int x, int y, int button);
        void mousePressed(int x, int y, int button);
        void mouseReleased(int x, int y, int button);
        void mouseEntered(int x, int y);
        void mouseExited(int x, int y);
        void windowResized(int w, int h);
        void dragEvent(ofDragInfo dragInfo);
        void gotMessage(ofMessage msg);
    
        // ofApp.cpp 에서 물 메쉬와 방패 메쉬를 그리는 함수를 분할해서 쪼개줄 것이므로, 각 함수의 메서드를 미리 선언해놓음.
        // 조명구조체는 결국 Light 구조체로부터 상속받은 애들 중 하나를 인자로 전달할 것이므로, 부모 구조체인 Light 로 타입을 지정해도 됨.
        void drawWater(Light& light, glm::mat4& proj, glm::mat4& view);
        void drawShield(Light& light, glm::mat4& proj, glm::mat4& view);
        void drawSkybox(glm::mat4& proj, glm::mat4& view); // ofApp.cpp 에서 큐브메쉬를 그리는 함수를 따로 추출하기 위해 선언한 메서드.
        void beginRenderingPointLights(); // 포인트라이트 패스 렌더링 시, 블렌딩모드와 깊이테스트 모드를 재설정하는 함수
        void endRenderingPointLights(); // 포인트라이트 패스 렌더링 완료 후, 블렌딩모드와 깊이테스트 모드를 초기화하는 함수 (자세한 설명은 ofApp.cpp 에서...)

        
        ofMesh shieldMesh; // shield.ply 모델링 파일을 로드해서 사용할 메쉬 객체 변수 선언
        ofMesh planeMesh; // plane.ply 모델링 파일을 로드해서 사용할 메쉬 객체 변수 선언
        ofMesh cubeMesh; // cube.ply 모델링 파일을 로드해서 사용할 메쉬 객체 변수 선언
        
        ofImage waterNrm; // plane.ply 에 씌워줄 노말맵을 로드하기 위한 이미지 객체 변수 선언
        
        ofImage diffuseTex; // shield.ply 에 씌워줄 디퓨즈 맵을 로드하기 위한 이미지 객체 변수 선언
        ofImage nrmTex; // shield.ply 에 씌워줄 스펙 맵을 로드하기 위한 이미지 객체 변수 선언
        ofImage specTex; // shield.ply 에 씌워줄 노말맵을 로드하기 위한 이미지 객체 변수 선언
        
        ofShader skyboxShader; // cube.ply 에 큐브맵 텍스쳐를 적용하기 위해 사용할 셰이더 객체 변수 선언
        ofShader dirLightShaders[2]; // 방패 및 물 메쉬에 각각 적용할 디렉셔널 라이트 쉐이더 객체 변수들이 담긴 배열 선언
        ofShader pointLightShaders[2]; // 방패 및 물 메쉬에 각각 적용할 포인트 라이트 쉐이더 객체 변수들이 담긴 배열 선언

        CameraData cam; // 카메라 위치 및 fov(시야각)의 현재 상태값을 나타내는 구조체를 타입으로 갖는 멤버변수 cam 선언
    
        ofxEasyCubemap cubemap; // 오픈프레임웍스는 큐브맵을 지원하지 않으므로, 큐브맵 로드 및 유니폼 변수 전송에 필요한 커스텀 클래스 객체 변수 선언
    
        // 각 조명 유형별 구조체 / 구조체 동적배열을 해더파일에 선언함. (이제 ofApp.cpp 내의 함수에서는 이 구조체/구조체 동적배열을 가져다가 써주면 됨.)
        DirectionalLight dirLight; // 디렉셔널 라이트 구조체 선언
        std::vector<PointLight> pointLights; // 포인트라이트 구조체를 담을 동적배열 선언 (동적배열 관련 필기 하단 참고)
};

/**
 virtual (가상함수)
 
 cpp 에서 virtual 키워드는
 자식 클래스에서 재정의(override) 할 것으로 예상되는
 부모 클래스의 멤버함수를 정의할 때 사용함,
 
 이런 걸 '가상함수' 라고 함.
 
 꼭 부모 클래스에만 붙이는 건 아니고,
 자식 클래스에도 멤버함수에 virtual 을 붙여서
 가상함수를 물려받아서 override 하고 있음을
 명시하기도 함.
 */

/**
 상속 접근 지정자 (public, protected, private)
 
 상속 접근 지정자는 자식 클래스에서 부모 클래스를 상속받을 때,
 부모 클래스 이름 왼쪽에 지정해주는 접근자임.
 
 이는 부모 클래스로부터 물려받는 멤버변수 및 함수들의
 접근 지정자를 어떻게 물려받을지 결정함.
 
 예를 들어, public 이라고 지정하면,
 public 보다 접근 범위가 넓은 애들(cpp 에서는 사실상 없음. public 이 접근 범위가 가장 넓으니까)을
 public 으로 접근하도록 하는 것이지.
 
 이 말은 결국, public 보다 접근 범위가 넓은 애들은
 사실상 없기 때문에, 부모 클래스에서 public, private, protected 로 지정한
 애들을 자식 클래스에서도 각각 동일한 접근자로 지정하도록 한 것.
 
 또 예를 들어, protected 라고 지정하면,
 protected 보다 접근 범위가 넓은 애들, 즉, 'public'을
 자식 클래스에서는 protected 로 물려받도록 지정해주는 것임.
 */

/**
 std::vector (동적배열)
 
 동적 배열이란 c++ 에서
 컴파일 이전에 배열의 길이 (메모리 크기)를 명시적으로 설정하지 않고도,
 런타임에 그때그때마다 배열안에 요소를
 동적으로 추가할 수 있도록 한 배열구조를 뜻함.
 
 마치 js 처럼 배열의 메모리를 자체적으로
 동적으로 처리한다는 것임!
 
 이걸 왜 사용하냐면,
 정적 멀티라이트에서는
 각 라이트유형마다 구조체배열의 크기가
 미리 정해져있고, 정해진 구조체배열 개수만큼
 라이트들을 생성할 수 있었잖아?
 
 그런데 이렇게 동적배열을 생성하면
 미리 정해진 배열의 크기가 없이
 그냥 cpp 파일에서든, 또는 렌더링 루프에서
 런타임에서도 원하는 만큼 라이트 구조체 요소를
 동적으로 추가할 수 있음.
 
 -> 그래서 이런 걸 '동적 멀티라이트' 라고 표현한 것!
 */
