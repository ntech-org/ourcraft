#pragma once

class Material {
public:
    static const Material air;
    static const Material ground;
    static const Material wood;
    static const Material rock;
    static const Material iron;
    static const Material water;
    static const Material lava;
    static const Material leaves;
    static const Material plants;
    static const Material sponge;
    static const Material cloth;
    static const Material fire;
    static const Material sand;
    static const Material glass;
    static const Material tnt;

    Material(bool liquid = false, bool solid = true, bool blocksGrass = true)
        : m_liquid(liquid), m_solid(solid), m_blocksGrass(blocksGrass) {}

    bool isLiquid() const { return m_liquid; }
    bool isSolid() const { return m_solid; }
    bool canBlockGrass() const { return m_blocksGrass; }

private:
    bool m_liquid;
    bool m_solid;
    bool m_blocksGrass;
};
