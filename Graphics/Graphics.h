#include "Parts/Common.h"

class Graphics
{
 public:
    enum class WindowType
    {
        Fullscreen,
        Windowed,
    };

    Graphics() = default;
    ~Graphics() = default;

    bool Init(WindowType _fullscreen);
    void Update();
    void Close();

    bool WantsToQuit();

    void ChangeMode(int x, int y, float jsss = Default_JSSS);

 private:
    void HandleMessages();
    void PrepareTextureForGPU();
};