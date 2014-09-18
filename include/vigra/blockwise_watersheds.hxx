#ifndef VIGRA_BLOCKWISE_WATERSHEDS_HXX_
#define VIGRA_BLOCKWISE_WATERSHEDS_HXX_

#include <vigra/multi_array.hxx>
#include <vigra/multi_gridgraph.hxx>
#include <vigra/blockify.hxx>
#include <vigra/blockwise_labeling.hxx>
#include <vigra/overlapped_blocks.hxx>

#include <limits>

namespace vigra
{

namespace blockwise_watersheds_detail
{

template <class DataArray, class DirectionsBlocksIterator>
void prepareBlockwiseWatersheds(const Overlaps<DataArray>& overlaps,
                                DirectionsBlocksIterator directions_blocks_begin,
                                NeighborhoodType neighborhood)
{
    static const unsigned int N = DataArray::actual_dimension;
    typedef typename MultiArrayShape<N>::type Shape;
    typedef typename DirectionsBlocksIterator::value_type DirectionsBlock;
    Shape shape = overlaps.shape();
    vigra_assert(shape == directions_blocks_begin.shape(), "");
    
    MultiCoordinateIterator<N> it(shape);
    MultiCoordinateIterator<N> end = it.getEndIterator();
    for( ; it != end; ++it)
    {
        DirectionsBlock directions_block = directions_blocks_begin[*it];
        OverlappingBlock<DataArray> data_block = overlaps[*it];
        
        typedef GridGraph<N, undirected_tag> Graph;
        typedef typename Graph::NodeIt GraphScanner;
        typedef typename Graph::OutArcIt NeighborIterator;
        
        Graph graph(data_block.block.shape(), neighborhood);
        for(GraphScanner node(graph); node != lemon::INVALID; ++node)
        {
            if(within(*node, data_block.inner_bounds))
            {
                typedef typename DataArray::value_type Data;
                Data lowest_neighbor = data_block.block[*node];
                
                typedef typename DirectionsBlock::value_type Direction;
                Direction lowest_neighbor_direction = std::numeric_limits<unsigned short>::max();
                
                for(NeighborIterator arc(graph, *node); arc != lemon::INVALID; ++arc)
                {
                    Shape neighbor_coordinates = graph.target(*arc);
                    Data neighbor_data = data_block.block[neighbor_coordinates];
                    if(neighbor_data < lowest_neighbor)
                    {
                        lowest_neighbor = neighbor_data;
                        lowest_neighbor_direction = arc.neighborIndex();
                    }
                }
                directions_block[*node - data_block.inner_bounds.first] = lowest_neighbor_direction;
            }
        }
    }
}

template <unsigned int N>
struct UnionFindWatershedEquality
{
    // FIXME: this graph object shouldn't be necessary, most functions (and state) of graph are not used
    // this probably needs some refactoring in GridGraph
    GridGraph<N, undirected_tag>* graph;

    template <class Shape>
    bool operator()(unsigned short u, const unsigned short v, const Shape& diff) const
    {
        static const unsigned short plateau_id = std::numeric_limits<unsigned short>::max();
        return (u == plateau_id && v == plateau_id) ||
               (u != plateau_id && graph->neighborOffset(u) == diff) ||
               (v != plateau_id && graph->neighborOffset(graph->oppositeIndex(v)) == diff);
    }

    struct WithDiffTag
    {};
};

}

template <unsigned int N, class Data, class S1,
                          class Label, class S2>
Label unionFindWatershedBlockwise(MultiArrayView<N, Data, S1> data,
                                  MultiArrayView<N, Label, S2> labels,
                                  NeighborhoodType neighborhood = DirectNeighborhood,
                                  const typename MultiArrayView<N, Data, S1>::difference_type& block_shape = 
                                          typename MultiArrayView<N, Data, S1>::difference_type(128))
{
    using namespace blockwise_watersheds_detail;

    typedef typename MultiArrayView<N, Data, S1>::difference_type Shape;
    Shape shape = data.shape();
    vigra_precondition(shape == labels.shape(), "shapes of data and labels do not match");
    
    MultiArray<N, unsigned short> directions(shape);
    
    MultiArray<N, MultiArrayView<N, unsigned short> > directions_blocks = blockify(directions, block_shape);

    Overlaps<MultiArrayView<N, Data, S1> > overlaps(data, block_shape, Shape(1), Shape(1));
    prepareBlockwiseWatersheds(overlaps, directions_blocks.begin(), neighborhood);
    GridGraph<N, undirected_tag> graph(data.shape(), neighborhood);
    UnionFindWatershedEquality<N> equal = {&graph};
    return labelMultiArrayBlockwise(directions, labels, LabelOptions().neighborhood(neighborhood).blockShape(block_shape), equal);
}

template <unsigned int N, class Data,
                          class Label>
Label unionFindWatershedBlockwise(const ChunkedArray<N, Data>& data,
                                  ChunkedArray<N, Label>& labels,
                                  NeighborhoodType neighborhood,
                                  ChunkedArray<N, unsigned short>& directions)
{
    using namespace blockwise_watersheds_detail;
    
    typedef typename ChunkedArray<N, Data>::shape_type Shape;
    Shape shape = data.shape();
    vigra_precondition(shape == labels.shape() && shape == directions.shape(), "shapes of data and labels do not match");
    Shape chunk_shape = data.chunkShape();
    vigra_precondition(chunk_shape == labels.chunkShape() && chunk_shape == directions.chunkShape(), "chunk shapes do not match");
    
    Overlaps<ChunkedArray<N, Data> > overlaps(data, data.chunkShape(), Shape(1), Shape(1));
    
    prepareBlockwiseWatersheds(overlaps, directions.chunk_begin(Shape(0), shape), neighborhood);
    
    GridGraph<N, undirected_tag> graph(shape, neighborhood);
    UnionFindWatershedEquality<N> equal = {&graph};
    return labelMultiArrayBlockwise(directions, labels, LabelOptions().neighborhood(neighborhood), equal);
}
template <unsigned int N, class Data,
                          class Label>
Label unionFindWatershedBlockwise(const ChunkedArray<N, Data>& data,
                                  ChunkedArray<N, Label>& labels,
                                  NeighborhoodType neighborhood = DirectNeighborhood)
{
    ChunkedArrayLazy<N, unsigned short> directions(data.shape(), data.chunkShape());
    return unionFindWatershedBlockwise(data, labels, neighborhood, directions);
}

}

#endif
