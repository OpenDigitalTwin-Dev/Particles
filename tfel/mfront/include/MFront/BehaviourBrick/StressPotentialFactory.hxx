/*!
 * \file   include/MFront/BehaviourBrick/StressPotentialFactory.hxx
 * \brief
 * \author Thomas Helfer
 * \date   15/03/2018
 */

#ifndef LIB_MFRONT_BEHAVIOURBRICK_STRESSPOTENTIALFACTORY_HXX
#define LIB_MFRONT_BEHAVIOURBRICK_STRESSPOTENTIALFACTORY_HXX

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "MFront/MFrontConfig.hxx"

namespace mfront::bbrick {

  // forward declaration
  struct StressPotential;

  //! \brief abstract factory for Stress potentials.
  struct MFRONT_VISIBILITY_EXPORT StressPotentialFactory {
    //! a simple alias
    using Generator = std::function<std::shared_ptr<StressPotential>()>;
    //! \return the uniq instance of the class
    static StressPotentialFactory& getFactory();
    /*!
     * \brief add a new generator
     * \param[in] n: name of the generator
     * \param[in] g: generator
     */
    void addGenerator(const std::string&, const Generator&);
    //! \return the list of available stress potentials
    std::vector<std::string> getRegistredStressPotentials() const;
    /*!
     * \brief generate a new stress potential
     * \param[in] n: name of the stress potential
     */
    std::shared_ptr<StressPotential> generate(const std::string&) const;

   private:
    //! \brief default constructor
    StressPotentialFactory();
    //! \brief move constructor (deleted)
    StressPotentialFactory(StressPotentialFactory&&) = delete;
    //! \brief copy constructor (deleted)
    StressPotentialFactory(const StressPotentialFactory&) = delete;
    //! \brief move assignement (deleted)
    StressPotentialFactory& operator=(StressPotentialFactory&&) = delete;
    //! \brief standard assignement(deleted)
    StressPotentialFactory& operator=(const StressPotentialFactory&) = delete;
    //! \brief destructor
    ~StressPotentialFactory();
    //! \brief generators
    std::map<std::string, Generator> generators;
  };  // end of struct StressPotentialFactory

}  // end of namespace mfront::bbrick

#endif /* LIB_MFRONT_BEHAVIOURBRICK_STRESSPOTENTIALFACTORY_HXX */
