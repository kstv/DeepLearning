//-------------------------------------------------------------------------------
// @author
//     Millhaus.Chen @time 2017/10/07 11:01
//-------------------------------------------------------------------------------
namespace mtl {

template<int... Layers>
CNN<Layers...>& CNN<Layers...>::init()
{
    mtl::for_each(m_weights, [](auto& weight) mutable
    {   weight.random(0, 1);
    });

    mtl::for_each(m_thresholds, [](auto& threshold) mutable
    {   threshold.random(0, 1);
    });

    return *this;
}

template<int... Layers>
template<class LX, class LY, class W, class T>
void CNN<Layers...>::forward(LX& layerX, LY& layerY, W& weight, T& threshold)
{
    layerY.multiply(layerX, weight); /// layerY = layerX * weight
    layerY.foreach([&layerX](auto& e){ return e / layerX.Col();}); /// 用于支持超大节点数
    layerY += threshold;
    layerY.foreach(m_sigfunc);
};

template<int... Layers>
template<class LX, class W, class T, class DX, class DY>
void CNN<Layers...>::backward(LX& layerX, W& weight, T& threshold, DX& deltaX, DY& deltaY)
{
    weight.adjustW(layerX, deltaY, m_learnrate);
    threshold.adjustT(deltaY, m_learnrate);
    /// 计算delta
    deltaX.mult_trans(weight, deltaY);
    layerX.foreach(m_dsigfunc);
    deltaX.hadamard(layerX);
};

template<int... Layers>
template<std::size_t... I>
bool CNN<Layers...>::train(const InMatrix& input, const OutMatrix& output, int times, double nor, std::index_sequence<I...>)
{
    /// 1. 输入归一化
    auto& layer0 = std::get<0>(m_layers);
    layer0 = input;
    layer0.normalize(nor);
    auto& layerN = std::get<N - 1>(m_layers);
    auto& deltaN = std::get<N - 1>(m_deltas);
    for(int i = 0; i < times; ++i)
    {   /// 2. 正向传播
        expander {(forward(std::get<I>(m_layers),
                           std::get<I + 1>(m_layers),
                           std::get<I>(m_weights),
                           std::get<I>(m_thresholds)),
                           0)...};
        /// 3. 判断误差
        double aberration = m_aberrmx.subtract(output, layerN).squariance() / 2;
        if (aberration < m_aberration) break;
        /// 4. 反向修正
        deltaN.hadamard(m_aberrmx, layerN.foreach(dlogsig));
        expander {(backward(std::get<N - I - 2>(m_layers),
                            std::get<N - I - 2>(m_weights),
                            std::get<N - I - 2>(m_thresholds),
                            std::get<N - I - 2>(m_deltas),
                            std::get<N - I - 1>(m_deltas)),
                            0)...};
    }
    return false;
}

template<int... Layers>
template<std::size_t... I>
double CNN<Layers...>::simulate(const InMatrix& input, OutMatrix& output, OutMatrix& expect, double nor, std::index_sequence<I...>)
{
    /// 1. 输入归一化
    auto& layer0 = std::get<0>(m_layers);
    layer0 = input;
    layer0.normalize(nor);
    /// 2. 正向传播
    expander {(forward(std::get<I>(m_layers),
                       std::get<I + 1>(m_layers),
                       std::get<I>(m_weights),
                       std::get<I>(m_thresholds)),
                       0)...};
    /// 3. 输出结果
    output = std::get<N - 1>(m_layers);

    /// 4. 判断误差
    double aberration = m_aberrmx.subtract(expect, output).squariance() / 2;

    return aberration;
}

}