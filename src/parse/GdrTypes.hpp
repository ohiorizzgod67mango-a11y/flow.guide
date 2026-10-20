#pragma once

#include <gdr/gdr.hpp>

namespace cgv {

struct PhysInput : gdr::Input<"Phys"> {
    float xPosition = 0.f;
    float yPosition = 0.f;
    float rotation = 0.f;
    double xVelocity = 0.0;
    double yVelocity = 0.0;
    bool extensionParsed = false;

    PhysInput() = default;

    PhysInput(uint64_t frame, uint8_t button, bool player2, bool down,
              float xpos, float ypos, float rot, double xvel, double yvel)
        : Input(frame, button, player2, down),
          xPosition(xpos), yPosition(ypos), rotation(rot),
          xVelocity(xvel), yVelocity(yvel), extensionParsed(true) {}

    void parseExtension(binary_reader& reader) override {
        reader >> xPosition >> yPosition >> rotation >> xVelocity >> yVelocity;
        extensionParsed = true;
    }

    void saveExtension(binary_writer& writer) const override {
        writer << xPosition << yPosition << rotation << xVelocity << yVelocity;
    }
};

struct GdrReplay : gdr::Replay<GdrReplay, PhysInput> {
    GdrReplay() = default;
    GdrReplay(std::string const& botName, int botVersion) : Replay(botName, botVersion) {}
};

} // namespace cgv
